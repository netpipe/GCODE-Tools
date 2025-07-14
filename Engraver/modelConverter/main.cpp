#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Stub function - Replace with your Assimp conversion logic
bool convertModel(const QString& inputFile, const QString& outputFile, const QString& formatId) {
    // Copy your Assimp code here
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene* scene = importer.ReadFile(inputFile.toStdString(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_SortByPType);

    if (!scene || !scene->mRootNode) {
        QMessageBox::critical(nullptr, "Error", QString("Failed to load model: ") + importer.GetErrorString());
        return false;
    }

    aiReturn result = exporter.Export(scene, formatId.toStdString(), outputFile.toStdString());
    if (result != aiReturn_SUCCESS) {
        QMessageBox::critical(nullptr, "Error", QString("Failed to export model: ") + exporter.GetErrorString());
        return false;
    }

    QMessageBox::information(nullptr, "Success", "Model converted successfully.");
    return true;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Assimp Model Converter");
    window.resize(400, 250);

    QVBoxLayout* layout = new QVBoxLayout(&window);

    QLabel* inputLabel = new QLabel("Input File:");
    QLineEdit* inputPath = new QLineEdit();
    QPushButton* browseInput = new QPushButton("Browse...");

    QLabel* outputLabel = new QLabel("Output File:");
    QLineEdit* outputPath = new QLineEdit();
    QPushButton* browseOutput = new QPushButton("Browse...");

    QLabel* formatLabel = new QLabel("Export Format:");
    QComboBox* formatDropdown = new QComboBox();

    QPushButton* convertButton = new QPushButton("Convert");

    layout->addWidget(inputLabel);
    layout->addWidget(inputPath);
    layout->addWidget(browseInput);

    layout->addWidget(outputLabel);
    layout->addWidget(outputPath);
    layout->addWidget(browseOutput);

    layout->addWidget(formatLabel);
    layout->addWidget(formatDropdown);

    layout->addWidget(convertButton);

    // Populate export formats
    Assimp::Exporter exporter;
    unsigned int count = exporter.GetExportFormatCount();
    for (unsigned int i = 0; i < count; ++i) {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
        formatDropdown->addItem(QString("%1 (%2)").arg(desc->description).arg(desc->fileExtension), desc->id);
    }

    QObject::connect(browseInput, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getOpenFileName(&window, "Select Input Model");
        if (!file.isEmpty())
            inputPath->setText(file);
    });

    QObject::connect(browseOutput, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getSaveFileName(&window, "Select Output File");
        if (!file.isEmpty())
            outputPath->setText(file);
    });

    QObject::connect(convertButton, &QPushButton::clicked, [&]() {
        QString inputFile = inputPath->text();
        QString outputFile = outputPath->text();
        QString formatId = formatDropdown->currentData().toString();

        if (inputFile.isEmpty() || outputFile.isEmpty()) {
            QMessageBox::warning(&window, "Input Error", "Please select both input and output files.");
            return;
        }

        convertModel(inputFile, outputFile, formatId);
    });

    window.show();
    return app.exec();
}
