// Qt 5.12 Single‑File G‑code Voice Player / Generator
// ---------------------------------------------------
// Features
//  - Captures microphone audio (mono, 16 kHz)
//  - Detects pitch via time‑domain autocorrelation
//  - Optionally quantizes to semitones
//  - Exports G‑code in two ways:
//      (A) M300 beeper (e.g., many Marlin firmwares)
//      (B) Stepper‑tone via back‑and‑forth X moves at feedrates matching pitch
//         (needs your machine's steps_per_mm)
//  - Bonus: “Circle” and “Square” tune generators (shape motion with speed melody)
//  - Single‑file demo, no external DSP libs required
//
// Build (qmake):
//   TEMPLATE = app
//   CONFIG += c++11
//   QT += widgets multimedia
//   SOURCES += main.cpp
//
// Notes
//  - If your firmware supports M300, prefer Mode "M300 (Beeper)".
//  - For motion mode, set correct steps_per_mm and travel amplitude.
//  - Keep generated feedrates within your machine’s safe range.
//
// DISCLAIMER: Use at your own risk. Verify output before running on real hardware.
// WIP not tested yet either
#include <QtWidgets>
#include <QtMultimedia>
#include <cmath>
#include <deque>
#include <algorithm>
// -------------------- Utility DSP --------------------
static float detectPitchAutocorr(const QVector<float>& s, int fs, float fmin=80.f, float fmax=1000.f) {
    if (s.isEmpty()) return 0.f;
    const int N = s.size();
    // Remove DC
    float mean = 0.f; for (int i=0;i<N;++i) mean += s[i]; mean /= std::max(1,N);
    // Hanning window & copy to float buffer
    std::vector<float> x(N);
    for (int i=0;i<N;++i) x[i] = (s[i]-mean) * (0.5f - 0.5f*std::cos(2*M_PI*i/(N-1)));
    // Autocorrelation (naive, but OK for 1024–4096 samples)
    const int minLag = int(std::floor(float(fs)/fmax));
    const int maxLag = int(std::ceil(float(fs)/fmin));
    if (maxLag >= N-1 || minLag <= 0) return 0.f;
    float bestR = 0.f; int bestLag = 0;
    for (int lag=minLag; lag<=maxLag; ++lag) {
        double num=0, den0=0, den1=0;
        const int L = N - lag;
        for (int i=0;i<L;++i) {
            float a = x[i];
            float b = x[i+lag];
            num += a*b;
            den0 += a*a;
            den1 += b*b;
        }
        float r = (den0>1e-9 && den1>1e-9) ? float(num / std::sqrt(den0*den1)) : 0.f;
        if (r > bestR) { bestR = r; bestLag = lag; }
    }
    if (bestLag<=0) return 0.f;
    float f = float(fs)/bestLag;
    // Simple confidence gate
    if (bestR < 0.6f) return 0.f; // weak periodicity
    return f;
}

static float freqToNearestSemitone(float f) {
    if (f<=0) return 0.f;
    int midi = int(std::round(69 + 12*std::log2(f/440.0f)));
    float q = 440.0f * std::pow(2.0f, (midi-69)/12.0f);
    return q;
}

// Convert a target frequency (Hz) to G1 feedrate (mm/min) that yields that step frequency
// given steps_per_mm and a back‑and‑forth stroke producing continuous steps.
static double freqToFeed(double freqHz, double steps_per_mm) {
    if (freqHz<=0 || steps_per_mm<=0) return 0.0;
    // step_freq = (feed_mm_per_min / 60) * steps_per_mm
    // => feed = (step_freq / steps_per_mm) * 60
    return (freqHz / steps_per_mm) * 60.0;
}

// Map RMS to a safe feed multiplier
static double rms(const QVector<float>& s){ double e=0; for(float v: s) e+=v*v; return s.isEmpty()?0.0:std::sqrt(e/s.size()); }

// -------------------- Audio Capture --------------------
class RingBuffer {
public:
    explicit RingBuffer(size_t cap=48000): cap_(cap) {}
    void push(const int16_t* data, int nsamples){
        for(int i=0;i<nsamples;++i){ buf_.push_back(data[i]); if(buf_.size()>cap_) buf_.pop_front(); }
    }
    QVector<float> lastWindow(int n, float invScale){
        n = std::min<int>(n, int(buf_.size()));
        QVector<float> out(n);
        auto it = buf_.end();
        for(int i=n-1;i>=0;--i){ --it; out[i] = (*it) * invScale; }
        return out;
    }
private:
    std::deque<int16_t> buf_;
    size_t cap_;
};

class MicTap : public QIODevice {
    Q_OBJECT
public:
    explicit MicTap(RingBuffer* rb, QObject* parent=nullptr): QIODevice(parent), rb_(rb) {}
    qint64 readData(char*, qint64) override { return 0; }
    qint64 writeData(const char* data, qint64 len) override {
        rb_->push(reinterpret_cast<const int16_t*>(data), int(len/2));
        return len;
    }
private:
    RingBuffer* rb_;
};

// -------------------- Main UI --------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(){
        setWindowTitle("G‑code Voice Player – Qt 5.12 Demo");
        resize(980, 700);

        // Widgets
        auto *central = new QWidget; setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);

        // Controls
        auto *row1 = new QHBoxLayout; layout->addLayout(row1);
        inputDeviceCombo = new QComboBox; row1->addWidget(new QLabel("Microphone:")); row1->addWidget(inputDeviceCombo, 1);
        startBtn = new QPushButton("Start Capture"); stopBtn = new QPushButton("Stop"); stopBtn->setEnabled(false);
        row1->addWidget(startBtn); row1->addWidget(stopBtn);

        auto *row2 = new QHBoxLayout; layout->addLayout(row2);
        modeCombo = new QComboBox; modeCombo->addItems({"M300 (Beeper)", "Stepper Tone (X back‑and‑forth)", "Circle Tune", "Square Tune"});
        row2->addWidget(new QLabel("Mode:")); row2->addWidget(modeCombo);

        semitoneCheck = new QCheckBox("Quantize to semitones"); semitoneCheck->setChecked(true);
        row2->addWidget(semitoneCheck);

        auto *row3 = new QHBoxLayout; layout->addLayout(row3);
        stepsPerMm = new QDoubleSpinBox; stepsPerMm->setRange(10, 2000); stepsPerMm->setValue(80); stepsPerMm->setDecimals(2);
        row3->addWidget(new QLabel("Steps/mm (X):")); row3->addWidget(stepsPerMm);
        strokeMm = new QDoubleSpinBox; strokeMm->setRange(0.1, 20.0); strokeMm->setValue(1.0); strokeMm->setDecimals(2);
        row3->addWidget(new QLabel("Stroke (mm):")); row3->addWidget(strokeMm);
        minFreq = new QDoubleSpinBox; minFreq->setRange(50, 2000); minFreq->setValue(100); row3->addWidget(new QLabel("Min Hz:")); row3->addWidget(minFreq);
        maxFreq = new QDoubleSpinBox; maxFreq->setRange(100, 8000); maxFreq->setValue(1000); row3->addWidget(new QLabel("Max Hz:")); row3->addWidget(maxFreq);

        auto *row4 = new QHBoxLayout; layout->addLayout(row4);
        windowMs = new QSpinBox; windowMs->setRange(20, 2000); windowMs->setValue(200);
        hopMs = new QSpinBox; hopMs->setRange(5, 1000); hopMs->setValue(120);
        row4->addWidget(new QLabel("Window (ms):")); row4->addWidget(windowMs);
        row4->addWidget(new QLabel("Hop (ms):")); row4->addWidget(hopMs);

        auto *row5 = new QHBoxLayout; layout->addLayout(row5);
        durationMs = new QSpinBox; durationMs->setRange(50, 20000); durationMs->setValue(150);
        row5->addWidget(new QLabel("G‑code note duration (ms):")); row5->addWidget(durationMs);
        baseFeed = new QDoubleSpinBox; baseFeed->setRange(10, 12000); baseFeed->setValue(1200);
        row5->addWidget(new QLabel("Base feed (mm/min, shapes):")); row5->addWidget(baseFeed);

        gcodeView = new QPlainTextEdit; gcodeView->setReadOnly(false);
        gcodeView->setPlaceholderText("Generated G‑code will appear here…");
        layout->addWidget(gcodeView, 1);

        auto *row6 = new QHBoxLayout; layout->addLayout(row6);
        saveBtn = new QPushButton("Save G‑code…"); clearBtn = new QPushButton("Clear");
        row6->addWidget(saveBtn); row6->addWidget(clearBtn);

        status = new QLabel; statusBar()->addWidget(status, 1);

        // Populate input devices
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        for (auto &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
            inputDeviceCombo->addItem(info.deviceName(), QVariant::fromValue(info));
        }
#endif

        connect(startBtn, &QPushButton::clicked, this, &MainWindow::startCapture);
        connect(stopBtn, &QPushButton::clicked, this, &MainWindow::stopCapture);
        connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveGcode);
        connect(clearBtn, &QPushButton::clicked, gcodeView, &QPlainTextEdit::clear);

        // Analysis timer
        connect(&hopTimer, &QTimer::timeout, this, &MainWindow::analyzeHop);

        // Initialize G‑code header
        gcodeView->appendPlainText("; G‑code Voice Player\nG90 ; absolute\nG21 ; mm\n");
    }

private slots:
    void startCapture(){
        if (audioIn) return;
        // Audio format
        QAudioFormat fmt; fmt.setSampleRate(fs); fmt.setChannelCount(1); fmt.setSampleSize(16);
        fmt.setCodec("audio/pcm"); fmt.setSampleType(QAudioFormat::SignedInt); fmt.setByteOrder(QAudioFormat::LittleEndian);

        QAudioDeviceInfo devInfo = QAudioDeviceInfo::defaultInputDevice();
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        int idx = inputDeviceCombo->currentIndex();
        if (idx>=0) devInfo = inputDeviceCombo->itemData(idx).value<QAudioDeviceInfo>();
#endif
        if (!devInfo.isFormatSupported(fmt)) {
            fmt = devInfo.nearestFormat(fmt);
        }

        audioIn.reset(new QAudioInput(devInfo, fmt, this));
        ring.reset(new RingBuffer(size_t(fmt.sampleRate()*4))); // ~4s buffer
        tap.reset(new MicTap(ring.get()));
        tap->open(QIODevice::WriteOnly);
        audioIn->start(tap.get());

        // Prepare state
        sampleRate = fmt.sampleRate();
        invScale = 1.0f / 32768.0f;
        hopTimer.start(hopMs->value());
        capturing = true;
        startBtn->setEnabled(false); stopBtn->setEnabled(true);
        status->setText(QString("Capturing @ %1 Hz").arg(sampleRate));
    }

    void stopCapture(){
        capturing = false;
        hopTimer.stop();
        if (audioIn){ audioIn->stop(); audioIn.reset(); }
        if (tap){ tap->close(); tap.reset(); }
        ring.reset();
        startBtn->setEnabled(true); stopBtn->setEnabled(false);
        status->setText("Stopped.");
    }

    void analyzeHop(){
        if (!ring) return;
        int win = int(std::round(windowMs->value() * 0.001 * sampleRate));
        QVector<float> window = ring->lastWindow(win, invScale);
        if (window.isEmpty()) return;

        float f = detectPitchAutocorr(window, sampleRate, float(minFreq->value()), float(maxFreq->value()));
        float amp = float(rms(window));
        if (semitoneCheck->isChecked()) f = freqToNearestSemitone(f);

        int dur = durationMs->value();
        if (modeCombo->currentIndex()==0) {
            // M300
            if (f>0) gcodeView->appendPlainText(QString("M300 S%1 P%2").arg(int(std::round(f))).arg(dur));
        } else if (modeCombo->currentIndex()==1) {
            // Stepper tone by feedrate, back‑and‑forth X moves
            double steps = stepsPerMm->value();
            double feed = freqToFeed(f, steps);
            // Scale by amplitude a bit to avoid silence; clamp safely
            double k = std::clamp(1.0 + 2.0*amp, 0.5, 3.0);
            feed = std::clamp(feed*k, 30.0, 12000.0);
            double dx = strokeMm->value();
            if (f>0) {
                gcodeView->appendPlainText(QString("G1 F%1").arg(feed,0,'f',1));
                gcodeView->appendPlainText(QString("G1 X%1").arg(back?0.0:dx,0,'f',3));
                gcodeView->appendPlainText(QString("G4 P%1").arg(dur));
                gcodeView->appendPlainText(QString("G1 X%1").arg(back?dx:0.0,0,'f',3));
                back = !back;
            }
        } else {
            // Shape tunes (circle/square) – synthesize a bar using the current pitch as root
            synthesizeShapeBar(f>0?f:440.0f, modeCombo->currentIndex()==2);
        }
    }

    void saveGcode(){
        QString fn = QFileDialog::getSaveFileName(this, "Save G‑code", QDir::homePath()+"/voice_tune.gcode", "G‑code (*.gcode *.nc)");
        if (fn.isEmpty()) return;
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly|QIODevice::Truncate)) { QMessageBox::warning(this, "Save", "Cannot write file."); return; }
        auto bytes = gcodeView->toPlainText().toUtf8();
        f.write(bytes); f.close();
        status->setText("Saved: "+fn);
    }

private:
    void synthesizeShapeBar(float rootHz, bool circle){
        // Simple 8‑note major scale pattern over one loop of shape
        static const int degrees[8] = {0,2,4,5,7,9,11,12};
        int dur = durationMs->value();
        double baseF = baseFeed->value();
        double steps = stepsPerMm->value();
        double R = std::max(0.5, strokeMm->value()); // reuse stroke as radius/half‑side
        gcodeView->appendPlainText("; --- Shape tune ---");
        if (circle) {
            // Approximate circle with 16 chords; modulate feed per note
            double cx=0, cy=0; gcodeView->appendPlainText("G0 X0 Y0");
            int N=16; for(int i=0;i<N;++i){
                float noteHz = rootHz * std::pow(2.0f, degrees[i%8]/12.0f);
                double feed = std::clamp(freqToFeed(noteHz, steps), 30.0, 6000.0);
                double ang0 = 2*M_PI*i/N, ang1 = 2*M_PI*(i+1)/N;
                double x1 = cx + R*std::cos(ang1), y1 = cy + R*std::sin(ang1);
                gcodeView->appendPlainText(QString("G1 F%1").arg(feed,0,'f',1));
                gcodeView->appendPlainText(QString("G2 X%1 Y%2 I%3 J%4").arg(x1,0,'f',3).arg(y1,0,'f',3).arg(-R*std::cos(ang0),0,'f',3).arg(-R*std::sin(ang0),0,'f',3));
                gcodeView->appendPlainText(QString("G4 P%1").arg(dur));
            }
        } else {
            // Square path with 8 notes along the perimeter
            double x=0,y=0; double s=2*R;
            gcodeView->appendPlainText("G0 X0 Y0");
            struct Seg{double dx,dy;};
            std::vector<Seg> segs={{s,0},{0,s},{-s,0},{0,-s}};
            int k=0; for (int rep=0; rep<2; ++rep){
                for (auto seg: segs){
                    float noteHz = rootHz * std::pow(2.0f, degrees[(k++)%8]/12.0f);
                    double feed = std::clamp(freqToFeed(noteHz, steps), 30.0, 6000.0);
                    x += seg.dx; y += seg.dy;
                    gcodeView->appendPlainText(QString("G1 F%1").arg(feed,0,'f',1));
                    gcodeView->appendPlainText(QString("G1 X%1 Y%2").arg(x,0,'f',3).arg(y,0,'f',3));
                    gcodeView->appendPlainText(QString("G4 P%1").arg(dur));
                }
            }
        }
        gcodeView->appendPlainText("; --- end shape ---");
    }

private:
    // UI
    QComboBox *inputDeviceCombo=nullptr, *modeCombo=nullptr;
    QCheckBox *semitoneCheck=nullptr;
    QDoubleSpinBox *stepsPerMm=nullptr, *strokeMm=nullptr, *baseFeed=nullptr;
    QDoubleSpinBox *minFreq=nullptr, *maxFreq=nullptr;
    QSpinBox *windowMs=nullptr, *hopMs=nullptr, *durationMs=nullptr;
    QPlainTextEdit *gcodeView=nullptr; QPushButton *startBtn=nullptr, *stopBtn=nullptr, *saveBtn=nullptr, *clearBtn=nullptr; QLabel *status=nullptr;

    // Audio
    std::unique_ptr<QAudioInput> audioIn; std::unique_ptr<RingBuffer> ring; std::unique_ptr<MicTap> tap;
    int sampleRate=16000; float invScale=1.0f/32768.0f; QTimer hopTimer; bool capturing=false; bool back=false;
    const int fs=16000;
};

int main(int argc, char** argv){
    QApplication app(argc, argv);
    MainWindow w; w.show();
    return app.exec();
}

#include "main.moc"
