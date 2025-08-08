#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QMimeData>
#include <QDropEvent>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

class AutoTraceFrontend : public QWidget {
    Q_OBJECT
public:
    AutoTraceFrontend(QWidget *parent = nullptr) : QWidget(parent) {
        setAcceptDrops(true);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *dropLabel = new QLabel("Drag & Drop an image here or click 'Open Image'");
        dropLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(dropLabel);

        QPushButton *openButton = new QPushButton("Open Image");
        layout->addWidget(openButton);

        // Sliders
        QHBoxLayout *despeckleLayout = new QHBoxLayout;
        despeckleLayout->addWidget(new QLabel("Despeckle Level"));
        despeckleLevelSlider = new QSlider(Qt::Horizontal);
        despeckleLevelSlider->setRange(0, 20);
        despeckleLevelSlider->setValue(0);
        despeckleLayout->addWidget(despeckleLevelSlider);
        layout->addLayout(despeckleLayout);

        QHBoxLayout *tightnessLayout = new QHBoxLayout;
        tightnessLayout->addWidget(new QLabel("Despeckle Tightness"));
        despeckleTightnessSlider = new QSlider(Qt::Horizontal);
        despeckleTightnessSlider->setRange(0, 200); // scaled x100 for float
        despeckleTightnessSlider->setValue(100);
        tightnessLayout->addWidget(despeckleTightnessSlider);
        layout->addLayout(tightnessLayout);

        // Centerline checkbox
        centerlineCheck = new QCheckBox("Centerline Trace");
        layout->addWidget(centerlineCheck);

        // Run button
        QPushButton *runButton = new QPushButton("Run Autotrace");
        layout->addWidget(runButton);

        // Log output
        outputLog = new QPlainTextEdit;
        outputLog->setReadOnly(true);
        layout->addWidget(outputLog);

        connect(openButton, &QPushButton::clicked, this, &AutoTraceFrontend::selectFile);
        connect(runButton, &QPushButton::clicked, this, &AutoTraceFrontend::runAutotrace);
    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }
    void dropEvent(QDropEvent *event) override {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            inputFile = urls.first().toLocalFile();
            log(QString("Loaded file: %1").arg(inputFile));
        }
    }

private slots:
    void selectFile() {
        QString file = QFileDialog::getOpenFileName(this, "Select Image", QString(),
                                                    "Images (*.bmp *.jpg *.jpeg *.png)");
        if (!file.isEmpty()) {
            inputFile = file;
            log(QString("Loaded file: %1").arg(inputFile));
        }
    }

    void runAutotrace() {
        if (inputFile.isEmpty()) {
            log("No file selected.");
            return;
        }

        QFileInfo fi(inputFile);
        QString ext = fi.suffix().toLower();

        QStringList args;
        args << QString("--input-format=%1").arg(ext);
        args << QString("--despeckle-level=%1").arg(despeckleLevelSlider->value());
        args << QString("--despeckle-tightness=%1").arg(despeckleTightnessSlider->value() / 100.0);
        if (centerlineCheck->isChecked())
            args << "--centerline";

        QString outputFile = fi.absolutePath() + "/" + fi.completeBaseName() + "_trace.svg";
        args << "--output-file" << outputFile;
        args << inputFile;

        log("Running: autotrace " + args.join(" "));

        QProcess proc;
        proc.start(QApplication::applicationDirPath() +"/autotrace", args);
        proc.waitForFinished(-1);

        log(proc.readAllStandardOutput());
        log(proc.readAllStandardError());

        log("Output saved to: " + outputFile);
    }

private:
    QString inputFile;
    QSlider *despeckleLevelSlider;
    QSlider *despeckleTightnessSlider;
    QCheckBox *centerlineCheck;
    QPlainTextEdit *outputLog;

    void log(const QString &s) {
        outputLog->appendPlainText(s);
    }
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AutoTraceFrontend w;
    w.resize(500, 400);
    w.show();
    return app.exec();
}
