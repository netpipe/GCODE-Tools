#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QMenuBar>
#include <QStatusBar>

class GCMCFrontend : public QMainWindow {
    Q_OBJECT

public:
    GCMCFrontend(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("GCMC Frontend");

        QWidget *central = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(central);

        QSplitter *splitter = new QSplitter(Qt::Vertical, this);

        editor = new QPlainTextEdit(this);
        editor->setPlaceholderText("Write or load GCMC source code here...");
        splitter->addWidget(editor);

        outputView = new QPlainTextEdit(this);
        outputView->setReadOnly(true);
        outputView->setPlaceholderText("Generated G-code output will appear here...");
        splitter->addWidget(outputView);

        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);

        layout->addWidget(splitter);

        QWidget *buttonWidget = new QWidget(this);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);

        QPushButton *runButton = new QPushButton("Run GCMC", this);
        QPushButton *saveButton = new QPushButton("Save G-code", this);
        buttonLayout->addWidget(runButton);
        buttonLayout->addWidget(saveButton);

        layout->addWidget(buttonWidget);

        setCentralWidget(central);

        statusBar();

        connect(runButton, &QPushButton::clicked, this, &GCMCFrontend::runGCMC);
        connect(saveButton, &QPushButton::clicked, this, &GCMCFrontend::saveGCode);

        QMenu *fileMenu = menuBar()->addMenu("&File");
        QAction *loadAction = new QAction("&Open GCMC Source...", this);
        fileMenu->addAction(loadAction);
        connect(loadAction, &QAction::triggered, this, &GCMCFrontend::loadGCMCFile);
    }

private slots:
    void runGCMC() {
        QString source = editor->toPlainText();
        if (source.trimmed().isEmpty()) {
            QMessageBox::warning(this, "No Source", "Please enter or load GCMC source code first.");
            return;
        }

        QString tempSourceFile = QDir::temp().filePath("gcmc_temp_input.ngc");
        QFile file(tempSourceFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to write temporary source file.");
            return;
        }
        QTextStream out(&file);
        out << source;
        file.close();

        QProcess gcmc;
        QStringList arguments;
        arguments << tempSourceFile;

        gcmc.start(QApplication::applicationDirPath() + "/gcmc", arguments);
        if (!gcmc.waitForStarted()) {
            QMessageBox::critical(this, "Error", "Failed to start GCMC process. Is it installed?");
            return;
        }
        gcmc.waitForFinished();

        QByteArray stdoutData = gcmc.readAllStandardOutput();
        QByteArray stderrData = gcmc.readAllStandardError();

        if (!stderrData.isEmpty()) {
            outputView->setPlainText("[ERROR]\n" + QString(stderrData));
            statusBar()->showMessage("GCMC finished with errors.", 5000);
        } else {
            outputView->setPlainText(QString(stdoutData));
            statusBar()->showMessage("GCMC finished successfully.", 5000);
        }
    }

    void saveGCode() {
        QString gcode = outputView->toPlainText();
        if (gcode.trimmed().isEmpty()) {
            QMessageBox::warning(this, "No Output", "No generated G-code to save.");
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(this, "Save G-code", "", "G-code Files (*.ngc *.gcode);;All Files (*)");
        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to save G-code file.");
            return;
        }

        QTextStream out(&file);
        out << gcode;
        file.close();

        statusBar()->showMessage("G-code saved to " + fileName, 5000);
    }

    void loadGCMCFile() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open GCMC Source", "", "GCMC Files (*.gcmc);;All Files (*)");
        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to load GCMC source file.");
            return;
        }

        QTextStream in(&file);
        editor->setPlainText(in.readAll());
        file.close();

        statusBar()->showMessage("Loaded " + fileName, 3000);
    }

private:
    QPlainTextEdit *editor;
    QPlainTextEdit *outputView;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    GCMCFrontend window;
    window.resize(800, 600);
    window.show();
    return app.exec();
}
