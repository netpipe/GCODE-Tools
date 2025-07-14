#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>

void printSupportedFormats() {
    Assimp::Exporter exporter;
    unsigned int formatCount = exporter.GetExportFormatCount();
    std::cout << "Supported export formats:\n";
    for (unsigned int i = 0; i < formatCount; ++i) {
        const aiExportFormatDesc* formatDesc = exporter.GetExportFormatDescription(i);
        std::cout << "- " << formatDesc->id << " (" << formatDesc->description << "): " << formatDesc->fileExtension << std::endl;
    }
}

bool convertModel(const std::string& inputFile, const std::string& outputFile, const std::string& exportFormatId) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputFile,
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_SortByPType);

    if (!scene || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return false;
    }

    Assimp::Exporter exporter;
    aiReturn result = exporter.Export(scene, exportFormatId, outputFile);

    if (result != aiReturn_SUCCESS) {
        std::cerr << "Error exporting model: " << exporter.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Model converted successfully to " << outputFile << std::endl;
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_model> <output_model> [export_format_id]\n";
        std::cout << "Default export format: obj\n\n";
        printSupportedFormats();
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string exportFormatId = "obj";  // Default format

    if (argc >= 4) {
        exportFormatId = argv[3];
    }

    if (!convertModel(inputFile, outputFile, exportFormatId)) {
        return 1;
    }

    return 0;
}
