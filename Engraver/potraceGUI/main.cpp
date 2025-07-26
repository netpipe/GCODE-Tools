#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QProcess>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QStyleFactory>
#include <QFileInfo>
#include <QImage>
#include <QString>
#include <QDebug>

class PotraceFrontend : public QMainWindow {
    Q_OBJECT

public:
    PotraceFrontend() {
        setWindowTitle("Potrace GUI Frontend");
        setMinimumSize(800, 600);
        setAcceptDrops(true);

        QWidget *central = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(central);

        // Input File Selection
        QHBoxLayout *inputLayout = new QHBoxLayout();
        inputLineEdit = new QLineEdit();
        QPushButton *browseBtn = new QPushButton("Browse Image...");
        inputLayout->addWidget(new QLabel("Input:"));
        inputLayout->addWidget(inputLineEdit);
        inputLayout->addWidget(browseBtn);
        layout->addLayout(inputLayout);

        // Output Format
        QHBoxLayout *formatLayout = new QHBoxLayout();
        formatCombo = new QComboBox();
        formatCombo->addItems({"svg", "pdf", "eps", "ps", "pgm", "dxf", "geojson"});
        formatLayout->addWidget(new QLabel("Output format:"));
        formatLayout->addWidget(formatCombo);
        layout->addLayout(formatLayout);

        // Options Group
        QGroupBox *optionsBox = new QGroupBox("Potrace Options");
        QVBoxLayout *optLayout = new QVBoxLayout();
        invertCheck = new QCheckBox("Invert colors");
        smoothCheck = new QCheckBox("Smooth curves (-a)");
        suppressCheck = new QCheckBox("Suppress speckles (-t)");
        pbmExport = new QCheckBox("PBMConvert");
        pbmExport->setChecked(1);
        QHBoxLayout *turdLayout = new QHBoxLayout();
        turdSize = new QLineEdit("2");
        turdLayout->addWidget(new QLabel("Turdsize:"));
        turdLayout->addWidget(turdSize);
        optLayout->addWidget(invertCheck);
        optLayout->addWidget(pbmExport);
        optLayout->addWidget(smoothCheck);
        optLayout->addWidget(suppressCheck);
        optLayout->addLayout(turdLayout);
        optionsBox->setLayout(optLayout);
        layout->addWidget(optionsBox);

        // Command Preview
        QHBoxLayout *previewLayout = new QHBoxLayout();
        commandPreview = new QLineEdit();
        commandPreview->setReadOnly(true);
        previewLayout->addWidget(new QLabel("Command:"));
        previewLayout->addWidget(commandPreview);
        layout->addLayout(previewLayout);

        // Convert Button
        QPushButton *runBtn = new QPushButton("Convert");
        layout->addWidget(runBtn);

        // Output Viewer
        QPushButton *openOutputBtn = new QPushButton("Open Output");
        layout->addWidget(openOutputBtn);

        // Console Output
        logOutput = new QPlainTextEdit();
        logOutput->setReadOnly(true);
        layout->addWidget(new QLabel("Console Output:"));
        layout->addWidget(logOutput);

        // Theme Toggle
        QPushButton *themeToggle = new QPushButton("Toggle Theme");
        layout->addWidget(themeToggle);

        setCentralWidget(central);

        // Connections
        connect(browseBtn, &QPushButton::clicked, this, &PotraceFrontend::selectFile);
        connect(runBtn, &QPushButton::clicked, this, &PotraceFrontend::runPotrace);
        connect(openOutputBtn, &QPushButton::clicked, this, &PotraceFrontend::openOutputFile);
        connect(themeToggle, &QPushButton::clicked, this, &PotraceFrontend::toggleTheme);

        connect(inputLineEdit, &QLineEdit::textChanged, this, &PotraceFrontend::updateCommandPreview);
        connect(formatCombo, &QComboBox::currentTextChanged, this, &PotraceFrontend::updateCommandPreview);
        connect(invertCheck, &QCheckBox::toggled, this, &PotraceFrontend::updateCommandPreview);
        connect(smoothCheck, &QCheckBox::toggled, this, &PotraceFrontend::updateCommandPreview);
        connect(suppressCheck, &QCheckBox::toggled, this, &PotraceFrontend::updateCommandPreview);
        connect(turdSize, &QLineEdit::textChanged, this, &PotraceFrontend::updateCommandPreview);

        updateCommandPreview();
    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent *event) override {
        const auto urls = event->mimeData()->urls();
        if (!urls.isEmpty())
            inputLineEdit->setText(urls.first().toLocalFile());
    }

private slots:

bool convertToPBM(const QString &inputPath, const QString &outputPath) {
    QImage image(inputPath);
    if (image.isNull()) {
        qDebug() << "Failed to load image:" << inputPath;
        return false;
    }

    // Convert to grayscale first (luminance)
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    // Threshold to 1-bit (black/white)
    QImage bw = gray.convertToFormat(QImage::Format_Mono);

    // Save as PBM
    if (!bw.save(outputPath, "PBM")) {
        qDebug() << "Failed to save PBM:" << outputPath;
        return false;
    }

    qDebug() << "PBM saved to:" << outputPath;
    return true;
}

    void selectFile() {
        QString file = QFileDialog::getOpenFileName(this, "Select Bitmap", QString(), "Bitmap Images (*.bmp *.pbm)");
        if (!file.isEmpty())
            inputLineEdit->setText(file);
    }

    void runPotrace() {
        QString file = inputLineEdit->text();
        if (file.isEmpty()) {
            log("No input file selected.");
            return;
        }

        QFileInfo fi(file);

        QString outputFormat = formatCombo->currentText();
        outputFile = fi.path() + "/" + fi.completeBaseName() + "." + outputFormat;

        QString cmd = QApplication::applicationDirPath() + "/potrace";

        if (invertCheck->isChecked()) cmd += " -i";
        if (smoothCheck->isChecked()) cmd += " -a 1";
        if (suppressCheck->isChecked()) cmd += " -t " + turdSize->text();
        else cmd += " -t 0";

        cmd += " -b " + outputFormat;
        cmd += " -o \"" + outputFile + "\"";
        if (pbmExport){
                    convertToPBM(fi.absoluteFilePath(),QApplication::applicationDirPath()  +"/tmp.pbm");
        cmd += " \"" + QApplication::applicationDirPath() + "/tmp.pbm" + "\"";
        }else{
            //cmd += " \"" + fi.absoluteFilePath() + "\"" + "-s"+ "\"";
            cmd += " \"" + fi.absoluteFilePath() + "\"";
        }

        log("Running: " + cmd);

        QProcess process;
        process.start(cmd);
        process.waitForFinished(-1);
        QString output = process.readAllStandardOutput();
        QString err = process.readAllStandardError();
        log(output + err);

        if (QFileInfo::exists(outputFile)) {
            log("Output file generated: " + outputFile);
        } else {
            log("Failed to generate output.");
        }
    }

    void openOutputFile() {
        if (!outputFile.isEmpty() && QFileInfo::exists(outputFile)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(outputFile));
        } else {
            log("No output file to open.");
        }
    }

    void updateCommandPreview() {
        QString file = inputLineEdit->text();
        if (file.isEmpty()) {
            commandPreview->setText("");
            return;
        }

        QFileInfo fi(file);
        QString outputFormat = formatCombo->currentText();
        QString previewFile = fi.path() + "/" + fi.completeBaseName() + "." + outputFormat;

        QString cmd = "potrace";
        if (invertCheck->isChecked()) cmd += " -i";
        if (smoothCheck->isChecked()) cmd += " -a 1";
        if (suppressCheck->isChecked()) cmd += " -t " + turdSize->text();
        else cmd += " -t 0";
        cmd += " -b " + outputFormat;
        cmd += " -o \"" + previewFile + "\"";
        cmd += " \"" + file + "\"";

        commandPreview->setText(cmd);
    }

    void toggleTheme() {
        static bool dark = false;
        dark = !dark;
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette palette;
        if (dark) {
            palette.setColor(QPalette::Window, QColor(53, 53, 53));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Base, QColor(25, 25, 25));
            palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(53, 53, 53));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Link, QColor(42, 130, 218));
        }
        qApp->setPalette(palette);
    }

    void log(const QString &msg) {
        logOutput->appendPlainText(msg);
    }

private:
    QLineEdit *inputLineEdit;
    QComboBox *formatCombo;
    QCheckBox *invertCheck;
    QCheckBox *smoothCheck;
    QCheckBox *suppressCheck;
    QCheckBox *pbmExport;
    QLineEdit *turdSize;
    QLineEdit *commandPreview;
    QPlainTextEdit *logOutput;
    QString outputFile;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    PotraceFrontend window;
    window.show();
    return app.exec();
}
