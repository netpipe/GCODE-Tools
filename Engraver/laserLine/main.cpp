#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include <QGroupBox>
#include <QPainter>

class LaserLineWidget : public QWidget {
    Q_OBJECT

public:
    LaserLineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Laser Line Overlay (Pure Qt)");

        QVBoxLayout *layout = new QVBoxLayout(this);

        QPushButton *loadBtn = new QPushButton("Load Image");
        QPushButton *saveBtn = new QPushButton("Save Image");
        QPushButton *processBtn = new QPushButton("Apply Lines");

        imageLabel = new QLabel("No image loaded");
        imageLabel->setMinimumSize(400, 300);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setStyleSheet("QLabel { background: #ddd; border: 1px solid #aaa; }");

        // Controls
        QGroupBox *controls = new QGroupBox("Settings");
        QVBoxLayout *ctrlLayout = new QVBoxLayout();

        QHBoxLayout *spacingLayout = new QHBoxLayout();
        QLabel *spacingLabel = new QLabel("Line spacing:");
        spacingSpin = new QSpinBox();
        spacingSpin->setRange(1, 50);
        spacingSpin->setValue(5);
        spacingLayout->addWidget(spacingLabel);
        spacingLayout->addWidget(spacingSpin);

        QHBoxLayout *thresholdLayout = new QHBoxLayout();
        QLabel *thresholdLabel = new QLabel("Brightness threshold:");
        thresholdSlider = new QSlider(Qt::Horizontal);
        thresholdSlider->setRange(0, 255);
        thresholdSlider->setValue(220);
        thresholdLayout->addWidget(thresholdLabel);
        thresholdLayout->addWidget(thresholdSlider);

        ctrlLayout->addLayout(spacingLayout);
        ctrlLayout->addLayout(thresholdLayout);
        controls->setLayout(ctrlLayout);

        layout->addWidget(loadBtn);
        layout->addWidget(imageLabel);
        layout->addWidget(controls);
        layout->addWidget(processBtn);
        layout->addWidget(saveBtn);

        connect(loadBtn, &QPushButton::clicked, this, &LaserLineWidget::loadImage);
        connect(processBtn, &QPushButton::clicked, this, &LaserLineWidget::applyLines);
        connect(saveBtn, &QPushButton::clicked, this, &LaserLineWidget::saveImage);
    }

private slots:
    void loadImage() {
        QString path = QFileDialog::getOpenFileName(this, "Open Image");
        if (path.isEmpty()) return;

        originalImage.load(path);
        processedImage = originalImage;
        updatePreview();
    }

    void applyLines() {
        if (originalImage.isNull()) {
            QMessageBox::warning(this, "Error", "Load an image first.");
            return;
        }

        int spacing = spacingSpin->value();
        int threshold = thresholdSlider->value();

        QImage img = originalImage.convertToFormat(QImage::Format_RGB32);

        int thickness = 3; // Adjust this to make lines wider

        for (int y = 0; y < img.height(); y += spacing) {
            for (int dy = 0; dy < thickness && (y + dy) < img.height(); ++dy) {
                for (int x = 0; x < img.width(); ++x) {
                    QColor color = img.pixelColor(x, y + dy);
                    int brightness = qGray(color.rgb());
                    if (brightness >= threshold) {
                        img.setPixelColor(x, y + dy, Qt::black);
                    }
                }
            }
        }



        processedImage = img;
        updatePreview();
    }

    void saveImage() {
        if (processedImage.isNull()) {
            QMessageBox::warning(this, "Error", "No image to save.");
            return;
        }

        QString path = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG (*.png);;JPEG (*.jpg)");
        if (!path.isEmpty()) {
            processedImage.save(path);
            QMessageBox::information(this, "Saved", "Image saved successfully.");
        }
    }

private:
    void updatePreview() {
        QPixmap pixmap = QPixmap::fromImage(processedImage).scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(pixmap);
    }

    QLabel *imageLabel;
    QSpinBox *spacingSpin;
    QSlider *thresholdSlider;
    QImage originalImage, processedImage;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    LaserLineWidget window;
    window.resize(500, 600);
    window.show();
    return app.exec();
}
