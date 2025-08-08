// autotrace_frontend.cpp
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QBoxLayout>
#include <QFileDialog>
#include <QImage>
#include <QProcess>
#include <QTextEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QTextStream>
#include <QByteArray>
/*
 Simple Autotrace front-end (Qt 5.12 single file)
 - Drag & drop or use "Open" to supply images
 - Converts to PBM (1-bit) using threshold
 - Runs autotrace --centerline -> SVG
 - Shows process output and opens resulting SVG
 NOTE: autotrace must be installed and in PATH
*/

class AutotraceWidget : public QWidget {
    Q_OBJECT
public:
    AutotraceWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Autotrace Centerline Frontend");
        setMinimumSize(720, 480);
        setAcceptDrops(true);

        auto *mainLay = new QVBoxLayout(this);

        // Top: filename / open / drop hint
        auto *topLay = new QHBoxLayout();
        fileEdit = new QLineEdit();
        fileEdit->setReadOnly(true);
        fileEdit->setPlaceholderText("Drop an image here or click Open...");
        QPushButton *openBtn = new QPushButton("Open");
        topLay->addWidget(fileEdit);
        topLay->addWidget(openBtn);
        mainLay->addLayout(topLay);

        // Center: preview label (shows small icon) + control group
        auto *midLay = new QHBoxLayout();

        preview = new QLabel();
        preview->setFixedSize(240, 240);
        preview->setFrameShape(QFrame::Box);
        preview->setAlignment(Qt::AlignCenter);
        preview->setText("Drop Image\n( BMP/PNG/JPG )");
        midLay->addWidget(preview);

        auto *opts = new QGroupBox("Options");
        auto *optsLay = new QVBoxLayout(opts);

        auto *threshLay = new QHBoxLayout();
        threshLay->addWidget(new QLabel("Threshold (0-255):"));
        thresholdSpin = new QSpinBox();
        thresholdSpin->setRange(0, 255);
        thresholdSpin->setValue(128);
        threshLay->addWidget(thresholdSpin);
        optsLay->addLayout(threshLay);

        centerlineCheck = new QCheckBox("Centerline trace (--centerline)");
        centerlineCheck->setChecked(true);
        optsLay->addWidget(centerlineCheck);

        smoothingCheck = new QCheckBox("Smooth output (autotrace smoothing)");
        smoothingCheck->setChecked(true);
        optsLay->addWidget(smoothingCheck);

        auto *scaleLay = new QHBoxLayout();
        scaleLay->addWidget(new QLabel("Scale (dpi for nanosvg/potrace) :"));
        dpiSpin = new QSpinBox();
        dpiSpin->setRange(10, 1200);
        dpiSpin->setValue(300);
        scaleLay->addWidget(dpiSpin);
        optsLay->addLayout(scaleLay);

        midLay->addWidget(opts);
        mainLay->addLayout(midLay);

        // Buttons: Convert & Trace, Choose Output, Open Output Folder
        auto *btnLay = new QHBoxLayout();
        traceBtn = new QPushButton("Trace (convert -> autotrace)");
        QPushButton *chooseOutBtn = new QPushButton("Choose Output Folder");
        openOutBtn = new QPushButton("Open Output Folder");
        openOutBtn->setEnabled(false);
        btnLay->addWidget(traceBtn);
        btnLay->addWidget(chooseOutBtn);
        btnLay->addWidget(openOutBtn);
        mainLay->addLayout(btnLay);

        // Output text area for logs
        outputLog = new QTextEdit();
        outputLog->setReadOnly(true);
        mainLay->addWidget(outputLog, 1);

        // Connections
        connect(openBtn, &QPushButton::clicked, this, &AutotraceWidget::openFile);
        connect(traceBtn, &QPushButton::clicked, this, &AutotraceWidget::onTrace);
        connect(chooseOutBtn, &QPushButton::clicked, this, &AutotraceWidget::chooseOutputFolder);
        connect(openOutBtn, &QPushButton::clicked, this, &AutotraceWidget::openOutputFolder);
    }

protected:
    // drag & drop
    void dragEnterEvent(QDragEnterEvent *e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override {
        const QList<QUrl> urls = e->mimeData()->urls();
        if (urls.isEmpty()) return;
        QString path = urls.first().toLocalFile();
        if (!path.isEmpty()) setFile(path);
    }

private slots:
    void openFile() {
        QString file = QFileDialog::getOpenFileName(this, "Open image", QString(), "Images (*.png *.bmp *.jpg *.jpeg *.tif *.tiff)");
        if (!file.isEmpty()) setFile(file);
    }

    void setFile(const QString &path) {
        currentFile = path;
        fileEdit->setText(path);
        QImage img(path);
        if (!img.isNull()) {
            QImage scaled = img.scaled(preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            preview->setPixmap(QPixmap::fromImage(scaled));
        } else {
            preview->setText("Preview not available");
        }
    }

    void chooseOutputFolder() {
        QString dir = QFileDialog::getExistingDirectory(this, "Choose output folder", outputFolder.isEmpty() ? QDir::homePath() : outputFolder);
        if (!dir.isEmpty()) {
            outputFolder = dir;
            openOutBtn->setEnabled(true);
            log(QString("Output folder: %1").arg(outputFolder));
        }
    }

    void openOutputFolder() {
        if (!outputFolder.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(outputFolder));
    }

    void onTrace() {
        if (currentFile.isEmpty()) {
            QMessageBox::warning(this, "No file", "Please open or drop an image first.");
            return;
        }
        if (outputFolder.isEmpty()) {
            QMessageBox::StandardButton ret = QMessageBox::question(this, "Output folder", "No output folder chosen. Use input file folder?");
            if (ret == QMessageBox::Yes) {
                outputFolder = QFileInfo(currentFile).absolutePath();
                openOutBtn->setEnabled(true);
                log(QString("Using input folder: %1").arg(outputFolder));
            } else return;
        }

        // Prepare names
        QString base = QFileInfo(currentFile).completeBaseName();
        QString pbmPath = QDir(outputFolder).filePath(base + ".pbm");
        QString svgPath = QDir(outputFolder).filePath(base + "_centerline.svg");

        // 1) Convert to PBM
        bool ok = convertToPBM(currentFile, pbmPath, thresholdSpin->value());
        if (!ok) {
            QMessageBox::critical(this, "Convert failed", "Failed to convert to PBM. See log.");
            return;
        }
        log(QString("Saved PBM: %1").arg(pbmPath));

        // 2) Run autotrace
        QStringList args;
        if (centerlineCheck->isChecked()) args << "--centerline";
       // if (smoothingCheck->isChecked()) args << "--smooth";
        // request svg output:
        args << "-output-format" << "svg";
        args << "-output-file" << svgPath;
        // input pbm last
        args << pbmPath;

        log(QString("Running autotrace: autotrace %1").arg(args.join(' ')));

        QProcess *proc = new QProcess(this);
        proc->setProgram("autotrace");
        proc->setArguments(args);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, proc, svgPath](int exitCode, QProcess::ExitStatus status){
            QString out = proc->readAllStandardOutput();
            QString err = proc->readAllStandardError();
            if (!out.isEmpty()) log(QString("[stdout]\n%1").arg(QString(out)));
            if (!err.isEmpty()) log(QString("[stderr]\n%1").arg(QString(err)));
            if (status == QProcess::NormalExit && exitCode == 0 && QFile::exists(svgPath)) {
                log(QString("Autotrace finished. SVG: %1").arg(svgPath));
                QDesktopServices::openUrl(QUrl::fromLocalFile(svgPath));
            } else {
                log(QString("Autotrace failed (exit %1). See stderr above.").arg(exitCode));
            }
            proc->deleteLater();
        });
        connect(proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e){
            log(QString("Process error: %1").arg((int)e));
        });
        proc->start();
    }

private:
    // Convert any image to 1-bit PBM using manual threshold for deterministic result
    bool convertToPBM(const QString &inPath, const QString &outPath, int threshold) {
        QImage img(inPath);
        if (img.isNull()) {
            log("Failed: load image");
            return false;
        }

        // convert to grayscale 8-bit
        QImage gray = img.convertToFormat(QImage::Format_Grayscale8);

        // create mono image with same size
        QImage mono(gray.size(), QImage::Format_Mono);
        mono.fill(1); // default white

        // manual threshold -> set black where luminance < threshold
        for (int y = 0; y < gray.height(); ++y) {
            const uchar *src = gray.constScanLine(y);
            uchar *dst = mono.scanLine(y);
            // For Format_Mono, bits are packed; easiest is to use setPixel
            for (int x = 0; x < gray.width(); ++x) {
                int lum = src[x];
                if (lum < threshold) mono.setPixel(x, y, 1); // black pixel in PBM output
                else mono.setPixel(x, y, 0); // white
            }
        }

        // Save as PBM via QImage writer. Some Qt builds accept "PBM" string.
        bool saved = mono.save(outPath, "PBM");
        if (!saved) {
            // fallback: write plain PBM manually (P4 binary)
            QFile f(outPath);
            if (!f.open(QIODevice::WriteOnly)) {
                log(QString("Failed to open PBM output: %1").arg(outPath));
                return false;
            }
            QTextStream ts; // not used; binary below
            // P4 binary PBM header
            // Old (Qt >= 5.15 only)
            // QByteArray header = QByteArray::asprintf("P4\n%d %d\n", mono.width(), mono.height());

            // Qt 5.12-safe version:
            QByteArray header = QString("P4\n%1 %2\n")
                                    .arg(mono.width())
                                    .arg(mono.height())
                                    .toUtf8();

            f.write(header);
            // pack bits per row
            for (int y = 0; y < mono.height(); ++y) {
                QByteArray row((mono.width() + 7) / 8, 0);
                for (int x = 0; x < mono.width(); ++x) {
                    bool black = mono.pixelIndex(x, y) ? true : false;
                    if (black) {
                        int byteIndex = x / 8;
                        int bitIndex = 7 - (x % 8);
                        unsigned char *raw = reinterpret_cast<unsigned char*>(row.data());
                        raw[byteIndex] |= (1 << bitIndex);
                    }
                }
                f.write(row);
            }
            f.close();
        }

        return QFile::exists(outPath);
    }

    void log(const QString &s) {
        outputLog->append(s.toHtmlEscaped());
    }

    // UI
    QLineEdit *fileEdit;
    QLabel *preview;
    QSpinBox *thresholdSpin;
    QSpinBox *dpiSpin;
    QSpinBox *dummy;
    QCheckBox *centerlineCheck;
    QCheckBox *smoothingCheck;
    QPushButton *traceBtn;
    QPushButton *openOutBtn;
    QTextEdit *outputLog;

    QString currentFile;
    QString outputFolder;
};

#include "main.moc"

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    AutotraceWidget w;
    w.show();
    return a.exec();
}
