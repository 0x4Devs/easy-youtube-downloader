// Easy Youtube Downloader — Qt6 Widgets GUI
// Features: DE/EN i18n, Settings-Dialog, Trim, versteckbarer Log, animierte Progress,
// Video-Info-Preview, Ordner-Oeffnen, Clipboard-Paste.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEasingCurve>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QStyleFactory>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTime>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QWidget>

// ==================== AnimatedProgressBar ==============================
// Grosser gerundeter Balken mit rotem Farbverlauf, zentrierter Prozentzahl
// (weiss + subtiler Schatten) und einem hellen Shimmer-Highlight das von links
// nach rechts wandert waehrend der Download laeuft.

class AnimatedProgressBar : public QProgressBar {
    Q_OBJECT
public:
    AnimatedProgressBar(QWidget* parent = nullptr) : QProgressBar(parent) {
        setTextVisible(false);
        setMinimumHeight(36);
        setMaximumHeight(36);
        shimmer_timer_ = new QTimer(this);
        shimmer_timer_->setInterval(16);      // ~60 FPS
        connect(shimmer_timer_, &QTimer::timeout, this, [this]{
            shimmer_phase_ += 0.012;
            if (shimmer_phase_ > 1.4) shimmer_phase_ = -0.4;
            update();
        });
    }

    void setAnimated(bool on) {
        if (on == animated_) return;
        animated_ = on;
        if (on) shimmer_timer_->start();
        else    { shimmer_timer_->stop(); shimmer_phase_ = -0.4; update(); }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = std::min<qreal>(10.0, r.height() / 2.0);

        // Hintergrund.
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x1B, 0x1F, 0x27));
        p.drawRoundedRect(r, radius, radius);

        double pct = (maximum() > 0) ? double(value()) / double(maximum()) : 0.0;
        if (pct < 0.0) pct = 0.0;
        if (pct > 1.0) pct = 1.0;

        if (pct > 0.0) {
            const qreal chunk_w = r.width() * pct;
            QRectF chunk = r;
            chunk.setWidth(chunk_w);

            // Roter Farbverlauf.
            QLinearGradient grad(chunk.topLeft(), chunk.bottomLeft());
            grad.setColorAt(0.0, QColor(0xEF, 0x44, 0x44));
            grad.setColorAt(1.0, QColor(0xB9, 0x1C, 0x1C));
            p.setBrush(grad);
            p.drawRoundedRect(chunk, radius, radius);

            // Shimmer-Highlight — nur wenn Download laeuft.
            if (animated_) {
                p.save();
                QPainterPath clip;
                clip.addRoundedRect(chunk, radius, radius);
                p.setClipPath(clip);

                const qreal band_x = shimmer_phase_ * chunk.width();
                QLinearGradient shimmer(band_x - 70, 0, band_x + 70, 0);
                shimmer.setColorAt(0.0, QColor(255, 255, 255,  0));
                shimmer.setColorAt(0.5, QColor(255, 255, 255, 80));
                shimmer.setColorAt(1.0, QColor(255, 255, 255,  0));
                p.setPen(Qt::NoPen);
                p.setBrush(shimmer);
                p.drawRect(chunk);
                p.restore();
            }
        }

        // Prozent-Text zentriert.
        QFont f = font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() + 0.5);
        p.setFont(f);
        const QString txt = QString::number(pct * 100.0, 'f', 1) + " %";
        // Sanfter Schatten fuer bessere Lesbarkeit auf beiden Hintergruenden.
        p.setPen(QColor(0, 0, 0, 110));
        p.drawText(rect().adjusted(1, 1, 1, 1), Qt::AlignCenter, txt);
        p.setPen(QColor(0xFF, 0xFF, 0xFF, 240));
        p.drawText(rect(), Qt::AlignCenter, txt);
    }

private:
    QTimer* shimmer_timer_ = nullptr;
    double  shimmer_phase_ = -0.4;
    bool    animated_       = false;
};

// ==================== L10n ==========================================

enum class Lang { DE = 0, EN = 1 };

struct L10n {
    const char* heading;
    const char* subtitle;
    const char* url_label;
    const char* url_hint;
    const char* paste;
    const char* format_label;
    const char* dir_label;
    const char* browse;
    const char* trim_check;
    const char* trim_start;
    const char* trim_end;
    const char* trim_hint;
    const char* download_start;
    const char* downloading;
    const char* cancel;
    const char* show_log;
    const char* hide_log;
    const char* open_folder;
    const char* settings_tooltip;
    const char* settings_title;
    const char* language_label;
    const char* language_de;
    const char* language_en;
    const char* save;
    const char* dialog_cancel;
    const char* fmt_best;
    const char* fmt_1080;
    const char* fmt_720;
    const char* fmt_mp3;
    const char* fetching_info;
    const char* done_ok;
    const char* done_err;
    const char* no_ytdlp;
    const char* invalid_time;
    const char* error;
};

static const L10n L_DE = {
    "Easy Youtube Downloader",
    "Video oder Audio von YouTube herunterladen",
    "Video-Link",
    "https://www.youtube.com/watch?v=...",
    "Einfügen",
    "Format",
    "Speicherort",
    "...",
    "Nur Zeitausschnitt herunterladen",
    "Start",
    "Ende",
    "Format:  Stunden:Minuten:Sekunden",
    "Download starten",
    "Wird heruntergeladen …",
    "Abbrechen",
    "Log anzeigen",
    "Log ausblenden",
    "Im Explorer öffnen",
    "Einstellungen",
    "Einstellungen",
    "Sprache",
    "Deutsch",
    "English",
    "Speichern",
    "Abbrechen",
    "Bestes MP4",
    "1080p MP4",
    "720p MP4",
    "MP3 (nur Audio)",
    "Info wird geladen …",
    "Fertig",
    "Fehler beim Download",
    "yt-dlp.exe wurde neben der Anwendung nicht gefunden.",
    "Endzeit muss nach Startzeit liegen.",
    "Fehler",
};

static const L10n L_EN = {
    "Easy Youtube Downloader",
    "Download video or audio from YouTube",
    "Video link",
    "https://www.youtube.com/watch?v=...",
    "Paste",
    "Format",
    "Save to",
    "...",
    "Download only a time range",
    "Start",
    "End",
    "Format:  hours:minutes:seconds",
    "Start download",
    "Downloading …",
    "Cancel",
    "Show log",
    "Hide log",
    "Open in Explorer",
    "Settings",
    "Settings",
    "Language",
    "Deutsch",
    "English",
    "Save",
    "Cancel",
    "Best MP4",
    "1080p MP4",
    "720p MP4",
    "MP3 (audio only)",
    "Fetching info …",
    "Done",
    "Download failed",
    "yt-dlp.exe was not found next to the application.",
    "End time must be after start time.",
    "Error",
};

static const L10n& L(Lang l) { return l == Lang::DE ? L_DE : L_EN; }

// ==================== gear icon =====================================

// Zahnrad-Icon aus Segoe MDL2 Assets (auf allen Win10/11 vorhanden).
// Fallback: Unicode ⚙ in Segoe UI Symbol.
static QIcon makeGearIcon(int px, QColor color) {
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(color);
    QFont f("Segoe MDL2 Assets");
    if (!QFontInfo(f).exactMatch()) f = QFont("Segoe UI Symbol");
    f.setPixelSize(int(px * 0.75));
    p.setFont(f);
    QString glyph = QFontInfo(f).family().contains("MDL2")
                    ? QString(QChar(0xE713))    // Settings icon
                    : QString(QChar(0x2699));    // ⚙
    p.drawText(pm.rect(), Qt::AlignCenter, glyph);
    return QIcon(pm);
}

// ==================== paths =========================================

static QString exeDir() {
    return QCoreApplication::applicationDirPath();
}

static QString ytdlpPath() {
    return QDir(exeDir()).filePath("yt-dlp.exe");
}

static QString ffmpegBundledDir() {
    QString candidate = QDir(exeDir()).filePath("ffmpeg.exe");
    return QFileInfo(candidate).exists() ? exeDir() : QString();
}

// ==================== Settings Dialog ================================

class SettingsDialog : public QDialog {
public:
    SettingsDialog(Lang current, QWidget* parent = nullptr) : QDialog(parent) {
        const L10n& l = L(current);
        setWindowTitle(l.settings_title);
        setModal(true);
        setMinimumWidth(360);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 24, 24, 20);
        root->setSpacing(14);

        auto* langLabel = new QLabel(l.language_label);
        langLabel->setProperty("class", "form-label");
        root->addWidget(langLabel);

        rbDe_ = new QRadioButton(l.language_de);
        rbEn_ = new QRadioButton(l.language_en);
        (current == Lang::DE ? rbDe_ : rbEn_)->setChecked(true);
        root->addWidget(rbDe_);
        root->addWidget(rbEn_);

        root->addStretch();

        auto* btns = new QHBoxLayout();
        btns->addStretch();
        auto* cancel = new QPushButton(l.dialog_cancel);
        auto* save   = new QPushButton(l.save);
        save->setProperty("class", "primary");
        save->setDefault(true);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(save,   &QPushButton::clicked, this, &QDialog::accept);
        btns->addWidget(cancel);
        btns->addWidget(save);
        root->addLayout(btns);
    }

    Lang chosenLang() const { return rbDe_->isChecked() ? Lang::DE : Lang::EN; }

private:
    QRadioButton* rbDe_;
    QRadioButton* rbEn_;
};

// ==================== MainWindow =====================================

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private slots:
    void onDownloadClicked();
    void onCancelClicked();
    void onPasteClicked();
    void onBrowseClicked();
    void onLogToggleClicked();
    void onSettingsClicked();
    void onOpenFolderClicked();
    void onTrimToggled(bool);
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus);
    void onUrlChanged();
    void onInfoFinished(int exitCode, QProcess::ExitStatus);

private:
    void buildUi();
    void applyStyle();
    void retranslate();
    void setLanguage(Lang);
    void setDownloading(bool);
    void animateProgressTo(int perMille);
    void appendLog(const QString&);
    void kickoffInfoFetch();
    QString buildOutputTemplate() const;

    Lang lang_ = Lang::DE;

    // header
    QLabel* heading_;
    QLabel* subtitle_;
    QToolButton* settingsBtn_;
    QFrame* separator_;

    // url row
    QLabel* urlLabel_;
    QLineEdit* urlEdit_;
    QPushButton* pasteBtn_;
    QLabel* videoInfoLabel_;

    // format + dir
    QLabel* formatLabel_;
    QComboBox* formatCombo_;
    QLabel* dirLabel_;
    QLineEdit* dirEdit_;
    QPushButton* browseBtn_;

    // trim
    QCheckBox* trimCheck_;
    QWidget* trimContainer_;
    QLabel* trimStartLabel_;
    QTimeEdit* trimStart_;
    QLabel* trimEndLabel_;
    QTimeEdit* trimEnd_;
    QLabel* trimHint_;

    // action
    QPushButton* downloadBtn_;
    QPushButton* cancelBtn_;
    AnimatedProgressBar* progressBar_;
    QLabel* progressLabel_;

    // log + open folder
    QPushButton* logToggleBtn_;
    QPushButton* openFolderBtn_;
    QPlainTextEdit* logView_;

    // process
    QProcess* process_ = nullptr;
    QProcess* infoProcess_ = nullptr;
    bool downloading_ = false;
    QString lastOutputFolder_;

    // progress animation
    QVariantAnimation* progressAnim_;
    int progressTargetPerMille_ = 0;

    // info fetch debounce
    QTimer* infoDebounce_;
};

MainWindow::MainWindow() {
    setWindowTitle("Easy Youtube Downloader");

    progressAnim_ = new QVariantAnimation(this);
    progressAnim_->setDuration(220);
    progressAnim_->setEasingCurve(QEasingCurve::OutCubic);
    connect(progressAnim_, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& v) { progressBar_->setValue(v.toInt()); });

    infoDebounce_ = new QTimer(this);
    infoDebounce_->setSingleShot(true);
    infoDebounce_->setInterval(600);
    connect(infoDebounce_, &QTimer::timeout, this, &MainWindow::kickoffInfoFetch);

    buildUi();
    applyStyle();

    // load persisted settings
    QSettings s("EasyYoutubeDownloader", "app");
    lang_ = (Lang)s.value("language", (int)Lang::DE).toInt();
    QString dir = s.value("downloadDir",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    dirEdit_->setText(QDir::toNativeSeparators(dir));
    formatCombo_->setCurrentIndex(s.value("formatIndex", 0).toInt());

    retranslate();

    // connections
    connect(urlEdit_,     &QLineEdit::textChanged,   this, &MainWindow::onUrlChanged);
    connect(downloadBtn_, &QPushButton::clicked,     this, &MainWindow::onDownloadClicked);
    connect(cancelBtn_,   &QPushButton::clicked,     this, &MainWindow::onCancelClicked);
    connect(pasteBtn_,    &QPushButton::clicked,     this, &MainWindow::onPasteClicked);
    connect(browseBtn_,   &QPushButton::clicked,     this, &MainWindow::onBrowseClicked);
    connect(logToggleBtn_,&QPushButton::clicked,     this, &MainWindow::onLogToggleClicked);
    connect(settingsBtn_, &QToolButton::clicked,     this, &MainWindow::onSettingsClicked);
    connect(openFolderBtn_,&QPushButton::clicked,    this, &MainWindow::onOpenFolderClicked);
    connect(trimCheck_,   &QCheckBox::toggled,       this, &MainWindow::onTrimToggled);
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [](int idx){ QSettings("EasyYoutubeDownloader","app").setValue("formatIndex", idx); });
    connect(dirEdit_,     &QLineEdit::editingFinished, this,
            [this]{ QSettings("EasyYoutubeDownloader","app").setValue("downloadDir", dirEdit_->text()); });

    // initial state
    onTrimToggled(false);
    logView_->hide();
    openFolderBtn_->hide();
    setDownloading(false);

    // Fenster auf minimale sinnvolle Groesse schrumpfen (kompakter Windows-Look).
    adjustSize();
    setMinimumWidth(520);
    resize(560, sizeHint().height());
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(8);

    // ---------- Header: Titel klein + Zahnrad rechts ----------
    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->setSpacing(6);
    heading_ = new QLabel();
    heading_->setProperty("class", "heading");
    settingsBtn_ = new QToolButton();
    settingsBtn_->setIcon(makeGearIcon(20, palette().color(QPalette::WindowText)));
    settingsBtn_->setIconSize(QSize(16, 16));
    settingsBtn_->setAutoRaise(true);
    settingsBtn_->setCursor(Qt::PointingHandCursor);
    headerRow->addWidget(heading_);
    headerRow->addStretch();
    headerRow->addWidget(settingsBtn_);
    root->addLayout(headerRow);

    subtitle_ = new QLabel();
    subtitle_->setProperty("class", "subtle");
    root->addWidget(subtitle_);

    separator_ = new QFrame();
    separator_->setFrameShape(QFrame::HLine);
    separator_->setFrameShadow(QFrame::Sunken);
    root->addWidget(separator_);

    // ---------- Form: kompaktes Label:Input Layout ----------
    auto* form = new QFormLayout();
    form->setContentsMargins(0, 4, 0, 4);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(6);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // URL-Zeile: LineEdit + Einfuegen-Button
    auto* urlRow = new QHBoxLayout();
    urlRow->setSpacing(6);
    urlEdit_ = new QLineEdit();
    urlEdit_->setClearButtonEnabled(true);
    pasteBtn_ = new QPushButton();
    urlRow->addWidget(urlEdit_, 1);
    urlRow->addWidget(pasteBtn_, 0);
    urlLabel_ = new QLabel();
    form->addRow(urlLabel_, urlRow);

    // Video-Info-Preview: unauffaellig unter der URL
    videoInfoLabel_ = new QLabel();
    videoInfoLabel_->setProperty("class", "subtle");
    videoInfoLabel_->setMinimumHeight(14);
    form->addRow("", videoInfoLabel_);

    // Format
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"", "", "", ""}); // via retranslate gefuellt
    formatCombo_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    formatCombo_->setMinimumContentsLength(18);
    formatLabel_ = new QLabel();
    form->addRow(formatLabel_, formatCombo_);

    // Speicherort: Edit + "..." Button
    auto* dirRow = new QHBoxLayout();
    dirRow->setSpacing(6);
    dirEdit_ = new QLineEdit();
    browseBtn_ = new QPushButton();
    browseBtn_->setFixedWidth(30);
    dirRow->addWidget(dirEdit_, 1);
    dirRow->addWidget(browseBtn_, 0);
    dirLabel_ = new QLabel();
    form->addRow(dirLabel_, dirRow);

    root->addLayout(form);

    // ---------- Zeitausschnitt als klassische GroupBox ----------
    auto* trimGroup = new QGroupBox();
    trimGroup->setObjectName("trimGroup");
    auto* trimGroupLayout = new QVBoxLayout(trimGroup);
    trimGroupLayout->setContentsMargins(10, 6, 10, 8);
    trimGroupLayout->setSpacing(6);

    trimCheck_ = new QCheckBox();
    trimGroupLayout->addWidget(trimCheck_);

    trimContainer_ = new QWidget();
    auto* trimRow = new QHBoxLayout(trimContainer_);
    trimRow->setContentsMargins(20, 0, 0, 0);
    trimRow->setSpacing(6);
    trimStartLabel_ = new QLabel();
    trimStart_ = new QTimeEdit(QTime(0,0,0));
    trimStart_->setDisplayFormat("HH:mm:ss");
    trimStart_->setFixedWidth(90);
    trimEndLabel_ = new QLabel();
    trimEnd_ = new QTimeEdit(QTime(0,0,30));
    trimEnd_->setDisplayFormat("HH:mm:ss");
    trimEnd_->setFixedWidth(90);
    trimHint_ = new QLabel();
    trimHint_->setProperty("class", "subtle");
    trimRow->addWidget(trimStartLabel_);
    trimRow->addWidget(trimStart_);
    trimRow->addSpacing(12);
    trimRow->addWidget(trimEndLabel_);
    trimRow->addWidget(trimEnd_);
    trimRow->addStretch();
    trimRow->addWidget(trimHint_);
    trimGroupLayout->addWidget(trimContainer_);

    root->addWidget(trimGroup);

    // ---------- Action: normaler Button rechts, kompakt ----------
    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(6);
    actionRow->addStretch();
    downloadBtn_ = new QPushButton();
    downloadBtn_->setProperty("class", "primary");
    downloadBtn_->setMinimumWidth(140);
    downloadBtn_->setCursor(Qt::PointingHandCursor);
    cancelBtn_ = new QPushButton();
    cancelBtn_->setMinimumWidth(140);
    cancelBtn_->setCursor(Qt::PointingHandCursor);
    actionRow->addWidget(downloadBtn_);
    actionRow->addWidget(cancelBtn_);
    root->addLayout(actionRow);

    // ---------- Progressbar (breit, mit zentrierter Prozentzahl) + Status ----------
    progressBar_ = new AnimatedProgressBar();
    progressBar_->setRange(0, 1000);
    progressBar_->setValue(0);
    root->addWidget(progressBar_);

    progressLabel_ = new QLabel();
    progressLabel_->setStyleSheet("color: #E6E9EE; font-size: 13px; font-weight: 500;");
    progressLabel_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    root->addWidget(progressLabel_);

    // ---------- Bottom: Log-Toggle links, Ordner-oeffnen rechts ----------
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(6);
    logToggleBtn_ = new QPushButton();
    logToggleBtn_->setFlat(true);
    logToggleBtn_->setCheckable(true);
    logToggleBtn_->setCursor(Qt::PointingHandCursor);
    openFolderBtn_ = new QPushButton();
    openFolderBtn_->setFlat(true);
    openFolderBtn_->setCursor(Qt::PointingHandCursor);
    bottomRow->addWidget(logToggleBtn_);
    bottomRow->addStretch();
    bottomRow->addWidget(openFolderBtn_);
    root->addLayout(bottomRow);

    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    logView_->setMinimumHeight(140);
    QFont mono("Consolas");
    if (!QFontInfo(mono).exactMatch()) mono = QFont("Courier New");
    mono.setPixelSize(11);
    logView_->setFont(mono);
    root->addWidget(logView_, 1);
}

void MainWindow::applyStyle() {
    // Nativer Windows-11-Style wenn verfuegbar; sonst Fusion.
    if (QStyleFactory::keys().contains("windows11", Qt::CaseInsensitive))
        QApplication::setStyle(QStyleFactory::create("windows11"));
    else if (QStyleFactory::keys().contains("windowsvista", Qt::CaseInsensitive))
        QApplication::setStyle(QStyleFactory::create("windowsvista"));
    else
        QApplication::setStyle(QStyleFactory::create("Fusion"));

    // System-Standard-Font (Segoe UI 9pt auf Windows).
    QApplication::setFont(QApplication::font());

    // Minimales QSS — nur die drei Sachen die nativ nicht funktionieren:
    //  1. Titel groesser, Subtitle gedimmt
    //  2. Roter Akzent fuer Download-Button (primary)
    //  3. Roter Progress-Chunk
    const QString qss = R"CSS(
    QLabel[class="heading"] {
        font-size: 14pt;
        font-weight: 600;
    }
    QLabel[class="subtle"] {
        color: #B8BEC8;
    }
    QPushButton[class="primary"] {
        background-color: #C4302B;
        color: white;
        font-weight: 600;
        padding: 4px 14px;
        border: 1px solid #A2251F;
        border-radius: 3px;
    }
    QPushButton[class="primary"]:hover    { background-color: #D33832; }
    QPushButton[class="primary"]:pressed  { background-color: #A2251F; }
    QPushButton[class="primary"]:disabled { background-color: palette(button); color: palette(mid); border: 1px solid palette(mid); }
    QProgressBar::chunk {
        background-color: #C4302B;
        border-radius: 2px;
    }
    QProgressBar {
        border: none;
        background: palette(base);
        border-radius: 3px;
    }
    )CSS";
    qApp->setStyleSheet(qss);
}

void MainWindow::retranslate() {
    const L10n& l = L(lang_);
    heading_->setText(l.heading);
    subtitle_->setText(l.subtitle);
    settingsBtn_->setToolTip(l.settings_tooltip);
    urlLabel_->setText(l.url_label);
    urlEdit_->setPlaceholderText(l.url_hint);
    pasteBtn_->setText(l.paste);
    formatLabel_->setText(l.format_label);
    dirLabel_->setText(l.dir_label);
    browseBtn_->setText(l.browse);

    formatCombo_->setItemText(0, l.fmt_best);
    formatCombo_->setItemText(1, l.fmt_1080);
    formatCombo_->setItemText(2, l.fmt_720);
    formatCombo_->setItemText(3, l.fmt_mp3);

    trimCheck_->setText(l.trim_check);
    trimStartLabel_->setText(l.trim_start);
    trimEndLabel_->setText(l.trim_end);
    trimHint_->setText(l.trim_hint);

    downloadBtn_->setText(l.download_start);
    cancelBtn_->setText(l.cancel);

    logToggleBtn_->setText(logToggleBtn_->isChecked() ? l.hide_log : l.show_log);
    openFolderBtn_->setText(l.open_folder);

    progressLabel_->setText("");
}

void MainWindow::setLanguage(Lang l) {
    lang_ = l;
    retranslate();
    QSettings("EasyYoutubeDownloader","app").setValue("language", (int)l);
}

// ---------- interaction slots ----------

void MainWindow::onSettingsClicked() {
    SettingsDialog dlg(lang_, this);
    if (dlg.exec() == QDialog::Accepted) setLanguage(dlg.chosenLang());
}

void MainWindow::onPasteClicked() {
    QString t = QGuiApplication::clipboard()->text().trimmed();
    if (!t.isEmpty()) urlEdit_->setText(t);
}

void MainWindow::onBrowseClicked() {
    QString start = dirEdit_->text();
    if (start.isEmpty()) start = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString chosen = QFileDialog::getExistingDirectory(this, L(lang_).dir_label, start);
    if (!chosen.isEmpty()) {
        dirEdit_->setText(QDir::toNativeSeparators(chosen));
        QSettings("EasyYoutubeDownloader","app").setValue("downloadDir", chosen);
    }
}

void MainWindow::onLogToggleClicked() {
    bool show = logToggleBtn_->isChecked();
    logView_->setVisible(show);
    logToggleBtn_->setText(show ? L(lang_).hide_log : L(lang_).show_log);
}

void MainWindow::onOpenFolderClicked() {
    if (lastOutputFolder_.isEmpty()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(lastOutputFolder_));
}

void MainWindow::onTrimToggled(bool on) {
    // Trim-Felder immer sichtbar — aber nur nutzbar wenn Checkbox an.
    trimStart_->setEnabled(on);
    trimEnd_->setEnabled(on);
    trimStartLabel_->setEnabled(on);
    trimEndLabel_->setEnabled(on);
    trimHint_->setEnabled(on);
}

void MainWindow::onUrlChanged() {
    videoInfoLabel_->clear();
    if (urlEdit_->text().trimmed().length() < 12) return;
    infoDebounce_->start();
}

// ---------- download ----------

QString MainWindow::buildOutputTemplate() const {
    return QDir(dirEdit_->text()).filePath("%(title)s [%(id)s].%(ext)s");
}

void MainWindow::onDownloadClicked() {
    QString url = urlEdit_->text().trimmed();
    QString dir = dirEdit_->text().trimmed();
    if (url.isEmpty() || dir.isEmpty()) return;

    QString ytdlp = ytdlpPath();
    if (!QFileInfo(ytdlp).exists()) {
        QMessageBox::warning(this, L(lang_).error, L(lang_).no_ytdlp);
        return;
    }

    QDir().mkpath(dir);
    lastOutputFolder_ = dir;
    openFolderBtn_->hide();
    logView_->clear();
    progressBar_->setValue(0);
    progressBar_->setAnimated(true);
    progressTargetPerMille_ = 0;
    progressLabel_->setText("");

    QStringList args;
    int fmt = formatCombo_->currentIndex();
    if (fmt == 0)      args << "-f" << "bv*+ba/b" << "--merge-output-format" << "mp4";
    else if (fmt == 1) args << "-f" << "bv*[height<=1080]+ba/b[height<=1080]" << "--merge-output-format" << "mp4";
    else if (fmt == 2) args << "-f" << "bv*[height<=720]+ba/b[height<=720]" << "--merge-output-format" << "mp4";
    else               args << "-x" << "--audio-format" << "mp3" << "--audio-quality" << "0";

    args << "-o" << buildOutputTemplate();
    args << "--newline" << "--no-part";

    if (trimCheck_->isChecked()) {
        QTime a = trimStart_->time();
        QTime b = trimEnd_->time();
        if (a >= b) {
            QMessageBox::warning(this, L(lang_).error, L(lang_).invalid_time);
            return;
        }
        args << "--download-sections"
             << QString("*%1-%2").arg(a.toString("HH:mm:ss"), b.toString("HH:mm:ss"));
        // Force keyframe cut for accurate trim on audio-only.
        args << "--force-keyframes-at-cuts";
    }

    QString ffdir = ffmpegBundledDir();
    if (!ffdir.isEmpty()) args << "--ffmpeg-location" << ffdir;

    args << url;

    if (process_) { process_->deleteLater(); }
    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessOutput);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);

    appendLog("$ yt-dlp " + args.join(' ') + "\n");
    process_->start(ytdlp, args);
    setDownloading(true);
}

void MainWindow::onCancelClicked() {
    if (process_ && process_->state() != QProcess::NotRunning) {
        process_->kill();
    }
}

static const QRegularExpression kRePct(R"(\[download\]\s+([\d.]+)%)");

void MainWindow::onProcessOutput() {
    QByteArray raw = process_->readAllStandardOutput();
    QString chunk = QString::fromLocal8Bit(raw);
    for (QString line : chunk.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts)) {
        appendLog(line + "\n");
        auto m = kRePct.match(line);
        if (m.hasMatch()) {
            double pct = m.captured(1).toDouble();
            int target = int(pct * 10.0);   // 0..1000
            if (target > progressTargetPerMille_) {
                animateProgressTo(target);
            }
            // Per-Fragment-Ruecksetzer ignorieren wir - Bar bleibt monoton steigend.
        }
    }
}

void MainWindow::onProcessFinished(int code, QProcess::ExitStatus) {
    setDownloading(false);
    progressBar_->setAnimated(false);
    if (code == 0) {
        animateProgressTo(1000);
        progressLabel_->setText(QString::fromUtf8(L(lang_).done_ok));
        openFolderBtn_->show();
    } else {
        progressLabel_->setText(QString::fromUtf8(L(lang_).done_err) + "  (exit " + QString::number(code) + ")");
    }
    appendLog(QString("\n=== exit %1 ===\n").arg(code));
}

// ---------- info fetch (async, debounced) ----------

void MainWindow::kickoffInfoFetch() {
    QString url = urlEdit_->text().trimmed();
    if (url.isEmpty()) return;
    QString ytdlp = ytdlpPath();
    if (!QFileInfo(ytdlp).exists()) return;

    if (infoProcess_) { infoProcess_->kill(); infoProcess_->deleteLater(); }
    infoProcess_ = new QProcess(this);
    infoProcess_->setProcessChannelMode(QProcess::SeparateChannels);
    connect(infoProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onInfoFinished);

    QStringList args;
    args << "--skip-download" << "--no-warnings"
         << "--print" << "%(title)s|%(duration_string)s"
         << url;
    videoInfoLabel_->setText(L(lang_).fetching_info);
    infoProcess_->start(ytdlp, args);
}

void MainWindow::onInfoFinished(int code, QProcess::ExitStatus) {
    if (code != 0) { videoInfoLabel_->clear(); return; }
    QByteArray out = infoProcess_->readAllStandardOutput().trimmed();
    QString line = QString::fromUtf8(out).split('\n').value(0);
    QStringList parts = line.split('|');
    if (parts.size() >= 2) {
        videoInfoLabel_->setText(QString("%1  ·  %2").arg(parts[0], parts[1]));
    } else if (!line.isEmpty()) {
        videoInfoLabel_->setText(line);
    } else {
        videoInfoLabel_->clear();
    }
}

// ---------- helpers ----------

void MainWindow::setDownloading(bool on) {
    downloading_ = on;
    downloadBtn_->setVisible(!on);
    cancelBtn_->setVisible(on);
    urlEdit_->setEnabled(!on);
    formatCombo_->setEnabled(!on);
    dirEdit_->setEnabled(!on);
    browseBtn_->setEnabled(!on);
    pasteBtn_->setEnabled(!on);
    trimCheck_->setEnabled(!on);
    trimStart_->setEnabled(!on);
    trimEnd_->setEnabled(!on);
    if (on) progressLabel_->setText(L(lang_).downloading);
}

void MainWindow::animateProgressTo(int target) {
    progressTargetPerMille_ = target;
    progressAnim_->stop();
    progressAnim_->setStartValue(progressBar_->value());
    progressAnim_->setEndValue(target);
    progressAnim_->start();
}

void MainWindow::appendLog(const QString& s) {
    logView_->moveCursor(QTextCursor::End);
    logView_->insertPlainText(s);
    logView_->moveCursor(QTextCursor::End);
}

// ==================== entry point ====================================

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Easy Youtube Downloader");
    QApplication::setOrganizationName("EasyYoutubeDownloader");
    QApplication::setWindowIcon(QIcon(":/ytdl.png"));   // Fenstertitel + Taskbar
    MainWindow w;
    w.show();
    return app.exec();
}

#ifdef _WIN32
// -mwindows: WinMain wird ohne Konsole aufgerufen; wir leiten an main() weiter.
extern "C" int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif

#include "gui.moc"
