#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDialog>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QListWidget>
#include <QSaveFile>
#include <QStackedWidget>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <algorithm>
#include <functional>

namespace {
constexpr char kTeal[] = "#168f83";
constexpr char kInk[] = "#193b36";
constexpr char kMuted[] = "#54736c";

QString prefsPath() { return QDir::homePath() + QStringLiteral("/.local/state/influent-danenone/visual-preferences.conf"); }

bool darkMode() {
    QFile file(prefsPath());
    if (!file.open(QIODevice::ReadOnly)) return false;
    return QString::fromUtf8(file.readAll()).contains(QStringLiteral("theme=dark"), Qt::CaseInsensitive);
}

QString accentColor() {
    QFile file(prefsPath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const QString &line : QString::fromUtf8(file.readAll()).split('\n')) {
            if (line.startsWith(QStringLiteral("accent="))) {
                const QString value = line.mid(QStringLiteral("accent=").size()).trimmed();
                if (QColor(value).isValid()) return value;
            }
        }
    }
    return QString::fromLatin1(kTeal);
}

int prefInt(const QString &key, int fallback) {
    QFile file(prefsPath());
    if (!file.open(QIODevice::ReadOnly)) return fallback;
    const QString text = QString::fromUtf8(file.readAll());
    const QRegularExpression expression(QStringLiteral("(?:^|\\n)") + QRegularExpression::escape(key) + QStringLiteral("=([0-9]+)"));
    const auto match = expression.match(text);
    return match.hasMatch() ? match.captured(1).toInt() : fallback;
}

QString assetPath(const QString &installed, const QString &local) {
    return QFileInfo::exists(installed) ? installed : local;
}

QString editionConfigPath() {
    const QString selected = qEnvironmentVariable("DANENONE_EDITION_CONFIG");
    if (!selected.isEmpty() && QFileInfo::exists(selected)) return selected;
    const QStringList candidates = {
        QStringLiteral("/etc/influent-danenone/edition.conf"),
        QStringLiteral("/home/ubuntu/danenone/editions/home.conf")
    };
    for (const QString &candidate : candidates) if (QFileInfo::exists(candidate)) return candidate;
    return {};
}

QString editionValue(const QString &key, const QString &fallback) {
    QFile file(editionConfigPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return fallback;
    for (const QString &line : QString::fromUtf8(file.readAll()).split('\n')) {
        if (line.startsWith(key + QStringLiteral("="))) return line.mid(key.size() + 1).trimmed();
    }
    return fallback;
}

QString updatePolicyLabel() {
    const QString policy = editionValue(QStringLiteral("UPDATE_POLICY"), QStringLiteral("manual"));
    if (policy == QStringLiteral("automatic")) return QStringLiteral("Actualizaciones\nAutomáticas");
    if (policy == QStringLiteral("managed")) return QStringLiteral("Actualizaciones\nAprobadas");
    if (policy == QStringLiteral("fast")) return QStringLiteral("Actualizaciones\nCanal rápido");
    if (policy == QStringLiteral("disabled")) return QStringLiteral("Actualizaciones\nDesactivadas · mantenimiento manual");
    return QStringLiteral("Actualizaciones\nManual");
}

QString storageStatus() {
    const QStorageInfo root = QStorageInfo::root();
    if (!root.isValid() || !root.isReady() || root.bytesTotal() <= 0) return QStringLiteral("Almacenamiento\nNo disponible");
    const double freeGb = static_cast<double>(root.bytesAvailable()) / (1024.0 * 1024.0 * 1024.0);
    const double totalGb = static_cast<double>(root.bytesTotal()) / (1024.0 * 1024.0 * 1024.0);
    return QStringLiteral("Almacenamiento\n") + QString::number(freeGb, 'f', 1) + QStringLiteral(" GB libres de ") + QString::number(totalGb, 'f', 1) + QStringLiteral(" GB");
}

QString currentUserName() {
    const QString user = qEnvironmentVariable("USER");
    return user.isEmpty() ? QDir::homePath().section('/', -1) : user;
}

QString userAvatarPath() {
    const QStringList candidates = {
        QDir::homePath() + QStringLiteral("/.face"),
        QDir::homePath() + QStringLiteral("/.face.icon"),
        QDir::homePath() + QStringLiteral("/.config/influent-danenone/avatar.png")
    };
    for (const QString &candidate : candidates) if (QFileInfo::exists(candidate)) return candidate;
    return assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-user.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-user.svg"));
}

bool commandAvailable(const QString &command) {
    return !QStandardPaths::findExecutable(command).isEmpty();
}

QString wallpaperPath() {
    QFile preferences(prefsPath());
    if (preferences.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const QString &line : QString::fromUtf8(preferences.readAll()).split('\n')) {
            if (line.startsWith(QStringLiteral("wallpaper="))) {
                const QString selected = line.mid(QStringLiteral("wallpaper=").size()).trimmed();
                if (QFileInfo::exists(selected)) return selected;
            }
        }
    }
    const QStringList candidates = {
        QStringLiteral("/usr/share/backgrounds/influent/danenone-river-wallpaper.jpg"),
        QStringLiteral("/home/ubuntu/danenone/native-shell/assets/danenone-river-wallpaper.jpg")
    };
    for (const QString &candidate : candidates) if (QFileInfo::exists(candidate)) return candidate;
    return {};
}

QString commandOutput(const QString &program, const QStringList &arguments, int timeout = 450) {
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeout)) return {};
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

class Hardware final : public QObject {
public:
    QString network() const {
        const QString output = commandOutput(QStringLiteral("nmcli"), {QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("TYPE,STATE,CONNECTION"), QStringLiteral("device")});
        for (const QString &line : output.split('\n')) {
            if (line.startsWith(QStringLiteral("wifi:connected")) || line.startsWith(QStringLiteral("ethernet:connected"))) return QStringLiteral("Red activa");
        }
        return output.isEmpty() ? QStringLiteral("Red no disponible") : QStringLiteral("Red desconectada");
    }

    QString bluetooth() const {
        const QString output = commandOutput(QStringLiteral("bluetoothctl"), {QStringLiteral("show")});
        if (!output.isEmpty()) {
            const auto powered = QRegularExpression(QStringLiteral("Powered:\\s+(yes|no)")).match(output);
            if (powered.hasMatch()) return powered.captured(1) == QStringLiteral("yes") ? QStringLiteral("Bluetooth activo") : QStringLiteral("Bluetooth apagado");
        }
        const QString rfkill = commandOutput(QStringLiteral("rfkill"), {QStringLiteral("list"), QStringLiteral("bluetooth")});
        return rfkill.isEmpty() ? QStringLiteral("Bluetooth no disponible") : QStringLiteral("Bluetooth detectado");
    }

    QString sound() const {
        const int value = volumePercent();
        return value < 0 ? QStringLiteral("Sonido no disponible") : QStringLiteral("Sonido ") + QString::number(value) + QStringLiteral(" %");
    }

    int volumePercent() const {
        const QString output = commandOutput(QStringLiteral("wpctl"), {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@")});
        const auto match = QRegularExpression(QStringLiteral("Volume: ([0-9]+(?:\\.[0-9]+)?)")).match(output);
        return match.hasMatch() ? std::clamp(qRound(match.captured(1).toDouble() * 100.0), 0, 100) : -1;
    }

    void setVolumePercent(int value) const {
        if (value >= 0) QProcess::startDetached(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QString::number(value / 100.0, 'f', 2)});
    }

    int brightnessPercent() const {
        const QString base = backlightBase();
        if (base.isEmpty()) return -1;
        QFile current(base + QStringLiteral("/brightness"));
        QFile maximum(base + QStringLiteral("/max_brightness"));
        if (!current.open(QIODevice::ReadOnly) || !maximum.open(QIODevice::ReadOnly)) return -1;
        const int currentValue = QString::fromUtf8(current.readAll()).trimmed().toInt();
        const int maximumValue = QString::fromUtf8(maximum.readAll()).trimmed().toInt();
        return maximumValue > 0 ? std::clamp(qRound(100.0 * currentValue / maximumValue), 0, 100) : -1;
    }

    void setBrightnessPercent(int value) const {
        const QString base = backlightBase();
        if (base.isEmpty() || value < 0) return;
        QFile maximum(base + QStringLiteral("/max_brightness"));
        QFile current(base + QStringLiteral("/brightness"));
        if (!maximum.open(QIODevice::ReadOnly) || !current.open(QIODevice::WriteOnly)) return;
        const int maximumValue = QString::fromUtf8(maximum.readAll()).trimmed().toInt();
        if (maximumValue > 0) current.write(QByteArray::number(qRound(maximumValue * value / 100.0)));
    }

    QString battery() const {
        QDir power(QStringLiteral("/sys/class/power_supply"));
        const QStringList devices = power.entryList({QStringLiteral("BAT*")}, QDir::Dirs);
        if (devices.isEmpty()) return QStringLiteral("Batería no disponible");
        const QString base = power.filePath(devices.first());
        QFile capacity(base + QStringLiteral("/capacity"));
        QFile status(base + QStringLiteral("/status"));
        if (!capacity.open(QIODevice::ReadOnly)) return QStringLiteral("Batería no disponible");
        const QString value = QString::fromUtf8(capacity.readAll()).trimmed();
        const QString state = status.open(QIODevice::ReadOnly) ? QString::fromUtf8(status.readAll()).trimmed() : QStringLiteral("estado desconocido");
        return QStringLiteral("Batería ") + value + QStringLiteral(" % · ") + state;
    }

    QString powerProfile() const {
        const QString value = commandOutput(QStringLiteral("powerprofilesctl"), {QStringLiteral("get")});
        return value.isEmpty() ? QStringLiteral("Perfil de energía no disponible") : QStringLiteral("Energía ") + value;
    }

private:
    QString backlightBase() const {
        QDir directory(QStringLiteral("/sys/class/backlight"));
        const QStringList devices = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        return devices.isEmpty() ? QString() : directory.filePath(devices.first());
    }
};

class GlassSurface final : public QWidget {
public:
    explicit GlassSurface(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);
        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(30);
        shadow->setOffset(0, 12);
        shadow->setColor(QColor(0, 0, 0, 62));
        setGraphicsEffect(shadow);
    }
    void setMaterial(const QColor &fill, const QColor &border) { m_fill = fill; m_border = border; update(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF bounds(0.5, 0.5, width() - 1.0, height() - 1.0);
        QPainterPath path;
        path.addRoundedRect(bounds, m_radius, m_radius);
        painter.fillPath(path, m_fill);
        painter.setPen(QPen(m_border, 1));
        painter.drawPath(path);
        painter.setPen(QPen(QColor(255, 255, 255, 42), 1));
        painter.drawLine(QPointF(m_radius, 1.5), QPointF(width() - m_radius, 1.5));
    }

private:
    int m_radius = 18;
    QColor m_fill = QColor(246, 252, 250, 218);
    QColor m_border = QColor(255, 255, 255, 175);
};

QString preferenceValue(const QString &key, const QString &fallback = {}) {
    QFile file(prefsPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return fallback;
    for (const QString &line : QString::fromUtf8(file.readAll()).split('\n')) {
        if (line.startsWith(key + QStringLiteral("="))) return line.mid(key.size() + 1).trimmed();
    }
    return fallback;
}

void persistPreference(const QString &key, const QString &value) {
    QFile input(prefsPath());
    QStringList lines;
    if (input.open(QIODevice::ReadOnly | QIODevice::Text)) lines = QString::fromUtf8(input.readAll()).split('\n', Qt::SkipEmptyParts);
    bool replaced = false;
    for (QString &line : lines) {
        if (line.startsWith(key + QStringLiteral("="))) { line = key + QStringLiteral("=") + value; replaced = true; }
    }
    if (!replaced) lines.append(key + QStringLiteral("=") + value);
    QDir().mkpath(QFileInfo(prefsPath()).absolutePath());
    QSaveFile output(prefsPath());
    if (output.open(QIODevice::WriteOnly | QIODevice::Text)) { output.write(lines.join('\n').toUtf8()); output.write("\n"); output.commit(); }
}

class ReorderListWidget final : public QListWidget {
public:
    using QListWidget::QListWidget;
    std::function<void()> onReordered;

protected:
    void dropEvent(QDropEvent *event) override {
        QListWidget::dropEvent(event);
        if (event->isAccepted() && onReordered) onReordered();
    }
};

class SettingsWindow final : public QDialog {
public:
    explicit SettingsWindow(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Configuración · Influent Danenone"));
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(900, 600);
        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(34); shadow->setOffset(0, 14); shadow->setColor(QColor(0, 0, 0, 75)); setGraphicsEffect(shadow);
        auto *surface = new GlassSurface(this);
        m_surface = surface;
        surface->setMaterial(darkMode() ? QColor(20, 28, 31, 238) : QColor(244, 250, 248, 224), darkMode() ? QColor(255, 255, 255, 70) : QColor(255, 255, 255, 185));
        surface->setGeometry(rect());
        surface->lower();
        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
        buildSidebar(root);
        buildPages(root);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QDialog::resizeEvent(event);
        if (m_surface) m_surface->setGeometry(rect());
    }

private:
    QString fg() const { return darkMode() ? QStringLiteral("#e7f6f0") : QStringLiteral("#193b36"); }
    QString secondary() const { return darkMode() ? QStringLiteral("#a7c5bd") : QStringLiteral("#54736c"); }

    void buildSidebar(QHBoxLayout *root) {
        auto *sidebar = new QWidget(this);
        sidebar->setFixedWidth(235);
        sidebar->setStyleSheet(QStringLiteral("background:rgba(255,255,255,0.13);border-right:1px solid rgba(255,255,255,0.20);"));
        auto *layout = new QVBoxLayout(sidebar);
        layout->setContentsMargins(18, 22, 14, 20);
        layout->setSpacing(10);
        auto *brand = new QLabel(QStringLiteral("Influent Danenone"));
        brand->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:%1;").arg(fg()));
        layout->addWidget(brand);
        auto *search = new QLineEdit;
        search->setPlaceholderText(QStringLiteral("Buscar una configuración"));
        search->setStyleSheet(QStringLiteral("QLineEdit{background:rgba(255,255,255,0.24);border:1px solid rgba(255,255,255,0.40);border-radius:10px;padding:8px;color:%1;} QLineEdit:focus{border:2px solid %2;}").arg(fg(), kTeal));
        layout->addWidget(search);
        m_categories = new QListWidget;
        m_categories->addItems({QStringLiteral("Inicio"), QStringLiteral("Sistema"), QStringLiteral("Hardware"), QStringLiteral("Red y conectividad"), QStringLiteral("Personalización"), QStringLiteral("Aplicaciones"), QStringLiteral("Usuarios"), QStringLiteral("Accesibilidad"), QStringLiteral("Actualizaciones")});
        m_categories->setCurrentRow(0);
        m_categories->setStyleSheet(QStringLiteral("QListWidget{background:transparent;border:none;color:%1;outline:none;} QListWidget::item{padding:9px 8px;border-radius:8px;} QListWidget::item:hover{background:rgba(255,255,255,0.24);} QListWidget::item:selected{background:rgba(22,143,131,0.24);border-left:3px solid %2;}").arg(fg(), kTeal));
        layout->addWidget(m_categories, 1);
        auto *close = new QPushButton(QStringLiteral("Cerrar"));
        close->setStyleSheet(QStringLiteral("QPushButton{background:rgba(255,255,255,0.20);border:1px solid rgba(255,255,255,0.44);border-radius:10px;padding:8px;color:%1;} QPushButton:hover{background:rgba(255,255,255,0.36);} QPushButton:focus{border:2px solid %2;}").arg(fg(), kTeal));
        connect(close, &QPushButton::clicked, this, &QDialog::close);
        layout->addWidget(close);
        root->addWidget(sidebar);
        connect(m_categories, &QListWidget::currentRowChanged, this, [this](int row) { if (m_pages) m_pages->setCurrentIndex(qBound(0, row, m_pages->count() - 1)); });
    }

    QLabel *heading(const QString &text, QVBoxLayout *layout) {
        auto *label = new QLabel(text);
        label->setStyleSheet(QStringLiteral("font-size:26px;font-weight:700;color:%1;").arg(fg()));
        layout->addWidget(label);
        return label;
    }

    QWidget *card(const QString &title, const QString &description, QVBoxLayout *layout) {
        auto *frame = new QFrame;
        frame->setStyleSheet(QStringLiteral("QFrame{background:rgba(255,255,255,0.18);border:1px solid rgba(255,255,255,0.34);border-radius:12px;}"));
        auto *box = new QVBoxLayout(frame);
        box->setContentsMargins(14, 10, 14, 10);
        auto *name = new QLabel(title);
        name->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:%1;").arg(fg()));
        auto *detail = new QLabel(description);
        detail->setStyleSheet(QStringLiteral("font-size:12px;color:%1;").arg(secondary()));
        box->addWidget(name); box->addWidget(detail);
        layout->addWidget(frame);
        return frame;
    }

    void buildPages(QHBoxLayout *root) {
        m_pages = new QStackedWidget(this);
        m_pages->addWidget(buildHomePage());
        m_pages->addWidget(buildSimplePage(QStringLiteral("Sistema"), {QStringLiteral("Pantalla"), QStringLiteral("Sonido"), QStringLiteral("Notificaciones"), QStringLiteral("Energía y batería")}));
        m_pages->addWidget(buildSimplePage(QStringLiteral("Hardware"), {QStringLiteral("Bluetooth"), QStringLiteral("Impresoras"), QStringLiteral("Entrada"), QStringLiteral("Almacenamiento")}));
        m_pages->addWidget(buildSimplePage(QStringLiteral("Red y conectividad"), {QStringLiteral("Wi-Fi"), QStringLiteral("VPN"), QStringLiteral("Proxy"), QStringLiteral("Compartir conexión")}));
        m_pages->addWidget(buildPersonalizationPage());
        m_pages->addWidget(buildSimplePage(QStringLiteral("Aplicaciones"), {QStringLiteral("Aplicaciones instaladas"), QStringLiteral("Aplicaciones predeterminadas"), QStringLiteral("Foundstore")}));
        m_pages->addWidget(buildSimplePage(QStringLiteral("Usuarios"), {QStringLiteral("Cuenta local"), QStringLiteral("Inicio de sesión"), QStringLiteral("Permisos")}));
        m_pages->addWidget(buildSimplePage(QStringLiteral("Accesibilidad"), {QStringLiteral("Texto y contraste"), QStringLiteral("Movimiento"), QStringLiteral("Transparencia")}));
        m_pages->addWidget(buildSimplePage(QStringLiteral("Actualizaciones"), {QStringLiteral("Estado del sistema"), QStringLiteral("Canal de actualización"), QStringLiteral("Puntos de restauración")}));
        root->addWidget(m_pages, 1);
    }

    QWidget *buildHomePage() {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(30, 28, 30, 24);
        layout->setSpacing(12);
        heading(QStringLiteral("Configuración"), layout);
        auto *summary = new QLabel(editionValue(QStringLiteral("DISPLAY_NAME"), QStringLiteral("Influent Danenone Home")) + QStringLiteral(" · ") + updatePolicyLabel().replace('\n', QStringLiteral(" · ")));
        summary->setStyleSheet(QStringLiteral("color:%1;font-size:13px;").arg(secondary()));
        layout->addWidget(summary);
        card(QStringLiteral("Sistema"), QStringLiteral("Pantalla, sonido, energía y notificaciones"), layout);
        card(QStringLiteral("Personalización"), QStringLiteral("Temas, colores, fondos y materiales de Danenone"), layout);
        card(QStringLiteral("Hardware"), QStringLiteral("Bluetooth, dispositivos, entrada y almacenamiento"), layout);
        card(QStringLiteral("Red y conectividad"), QStringLiteral("Wi-Fi, VPN, proxy y estado de red"), layout);
        layout->addStretch(1);
        auto *note = new QLabel(QStringLiteral("El notch permanece vacío y las superficies respetan las preferencias del OOBE."));
        note->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(secondary()));
        layout->addWidget(note);
        return page;
    }

    QWidget *buildSimplePage(const QString &title, const QStringList &items) {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(30, 28, 30, 24);
        layout->setSpacing(10);
        heading(title, layout);
        for (const QString &item : items) card(item, QStringLiteral("Configuración disponible para este dispositivo"), layout);
        layout->addStretch(1);
        return page;
    }

    QPushButton *choiceButton(const QString &text, const std::function<void()> &callback) {
        auto *button = new QPushButton(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral("QPushButton{background:rgba(255,255,255,0.20);border:1px solid rgba(255,255,255,0.42);border-radius:10px;padding:8px;color:%1;text-align:left;} QPushButton:hover{background:rgba(255,255,255,0.36);} QPushButton:focus{border:2px solid %2;}").arg(fg(), kTeal));
        connect(button, &QPushButton::clicked, this, callback);
        return button;
    }

    QWidget *buildPersonalizationPage() {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(30, 28, 30, 24);
        layout->setSpacing(10);
        heading(QStringLiteral("Personalización"), layout);
        auto *preview = new QLabel;
        preview->setFixedHeight(150);
        preview->setScaledContents(true);
        preview->setPixmap(QPixmap(wallpaperPath()));
        preview->setStyleSheet(QStringLiteral("border:1px solid rgba(255,255,255,0.48);border-radius:14px;"));
        layout->addWidget(preview);
        auto *themeTitle = new QLabel(QStringLiteral("Tema de Danenone"));
        themeTitle->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:%1;").arg(fg()));
        layout->addWidget(themeTitle);
        auto *themes = new QHBoxLayout;
        themes->addWidget(choiceButton(QStringLiteral("Claro · Arroyo"), [this] { persistPreference(QStringLiteral("theme"), QStringLiteral("light")); }));
        themes->addWidget(choiceButton(QStringLiteral("Oscuro · Piedra"), [this] { persistPreference(QStringLiteral("theme"), QStringLiteral("dark")); }));
        themes->addWidget(choiceButton(QStringLiteral("Automático"), [this] { persistPreference(QStringLiteral("theme"), QStringLiteral("auto")); }));
        layout->addLayout(themes);
        auto *accentTitle = new QLabel(QStringLiteral("Color de acento"));
        accentTitle->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:%1;").arg(fg()));
        layout->addWidget(accentTitle);
        auto *colors = new QHBoxLayout;
        for (const QString &color : {QStringLiteral("#168f83"), QStringLiteral("#2e8b72"), QStringLiteral("#2f76a8"), QStringLiteral("#b4793b")}) {
            auto *swatch = new QPushButton;
            swatch->setFixedSize(38, 30);
            swatch->setStyleSheet(QStringLiteral("QPushButton{background:%1;border:2px solid rgba(255,255,255,0.70);border-radius:8px;} QPushButton:focus{border:2px solid #193b36;}").arg(color));
            connect(swatch, &QPushButton::clicked, this, [color] { persistPreference(QStringLiteral("accent"), color); });
            colors->addWidget(swatch);
        }
        colors->addStretch(1);
        layout->addLayout(colors);
        auto *wallpaper = choiceButton(QStringLiteral("Elegir fondo de pantalla…"), [this] {
            const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Elegir fondo"), QDir::homePath(), QStringLiteral("Imágenes (*.png *.jpg *.jpeg *.webp)"));
            if (!path.isEmpty()) persistPreference(QStringLiteral("wallpaper"), path);
        });
        layout->addWidget(wallpaper);
        layout->addStretch(1);
        return page;
    }

    GlassSurface *m_surface = nullptr;
    QListWidget *m_categories = nullptr;
    QStackedWidget *m_pages = nullptr;
};

class ShellWindow final : public QMainWindow {
public:
    ShellWindow() {
        setWindowTitle(QStringLiteral("Influent Danenone"));
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        m_dark = darkMode();
        m_hardware = new Hardware;
        buildDesktop();
        buildLauncher();
        buildControlCenter();
        buildTaskbar();
        refreshHardware();
        m_refresh = new QTimer(this);
        connect(m_refresh, &QTimer::timeout, this, [this] { refreshHardware(); });
        m_refresh->start(3000);
        QTimer::singleShot(0, this, [this] { syncSceneGeometry(); });
    }

    void showSettingsForTest() { openSettings(); }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QMainWindow::resizeEvent(event);
        if (!m_desktop) return;
        syncSceneGeometry();
        if (m_launcher && m_launcher->isVisible()) centerLauncher();
        if (m_control && m_control->isVisible()) placeControl();
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->modifiers().testFlag(Qt::ControlModifier) && (event->key() == Qt::Key_K || event->key() == Qt::Key_Space)) {
            toggleLauncher();
            event->accept();
            return;
        }
        QMainWindow::keyPressEvent(event);
    }

private:
    QString textColor() const { return m_dark ? QStringLiteral("#e7f6f0") : QStringLiteral("#193b36"); }
    QString mutedColor() const { return m_dark ? QStringLiteral("#a7c5bd") : QStringLiteral("#54736c"); }

    QPushButton *iconButton(const QIcon &icon, const QString &tip, const std::function<void()> &callback) {
        auto *button = new QPushButton;
        button->setIcon(icon.isNull() ? style()->standardIcon(QStyle::SP_FileIcon) : icon);
        button->setIconSize(QSize(25, 25));
        button->setToolTip(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setFixedSize(48, 46);
        button->setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:1px solid transparent;border-radius:12px;color:%1;} QPushButton:hover{background:rgba(255,255,255,0.27);border-color:rgba(255,255,255,0.45);} QPushButton:focus{border:2px solid %2;} QPushButton:pressed{background:rgba(255,255,255,0.44);}").arg(textColor(), kTeal));
        connect(button, &QPushButton::clicked, this, callback);
        return button;
    }

    void syncSceneGeometry() {
        if (!m_desktop) return;
        const QRect scene = m_desktop->rect();
        if (m_wallpaper) m_wallpaper->setGeometry(scene);
        if (m_desktopLayer) m_desktopLayer->setGeometry(scene);
        layoutDesktopShortcuts();
        if (m_notch) m_notch->move((m_desktop->width() - m_notch->width()) / 2, 0);
        if (m_launcher && m_launcher->isVisible()) centerLauncher();
        if (m_control && m_control->isVisible()) placeControl();
    }

    void buildDesktop() {
        auto *root = new QWidget;
        root->setStyleSheet(QStringLiteral("QWidget#root{background:transparent;}"));
        root->setObjectName(QStringLiteral("root"));
        setCentralWidget(root);
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        m_desktop = new QWidget;
        m_desktop->setObjectName(QStringLiteral("desktop"));
        rootLayout->addWidget(m_desktop, 1);

        m_wallpaper = new QLabel(m_desktop);
        m_wallpaper->setScaledContents(true);
        m_wallpaper->setPixmap(QPixmap(wallpaperPath()));
        auto *blur = new QGraphicsBlurEffect(m_wallpaper);
        blur->setBlurRadius(7.0);
        blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
        m_wallpaper->setGraphicsEffect(blur);
        m_wallpaper->lower();

        m_desktopLayer = new QWidget(m_desktop);
        m_shortcutsGrid = new QGridLayout(m_desktopLayer);
        m_shortcutsGrid->setContentsMargins(22, 24, 0, 24);
        m_shortcutsGrid->setHorizontalSpacing(10);
        m_shortcutsGrid->setVerticalSpacing(10);
        m_shortcutsGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        addShortcut(QStringLiteral("Firefox"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-browser.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-browser.svg"))), QStringLiteral("firefox"));
        addShortcut(QStringLiteral("Dolphin"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-files.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-files.svg"))), QStringLiteral("dolphin"));
        addShortcut(QStringLiteral("VLC"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-media.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-media.svg"))), QStringLiteral("vlc"));
        addShortcut(QStringLiteral("Foundstore"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-foundstore.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-foundstore.svg"))), QStringLiteral("xdg-open https://foundstore.onrender.com"));
        addShortcut(QStringLiteral("Papelera"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-trash.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-trash.svg"))), QStringLiteral("gio open trash:///"));
        m_desktopLayer->raise();
        layoutDesktopShortcuts();

        m_notch = new QWidget(m_desktop);
        m_notch->setFixedSize(prefInt(QStringLiteral("notch_width"), 360), prefInt(QStringLiteral("notch_height"), 34));
        m_notch->setStyleSheet(QStringLiteral("background:#081118;border-bottom-left-radius:24px;border-bottom-right-radius:24px;"));
        m_notch->raise();

        rootLayout->addStretch(0);
    }

    void addShortcut(const QString &name, const QIcon &icon, const QString &command) {
        auto *button = new QPushButton(name);
        button->setIcon(icon.isNull() ? style()->standardIcon(QStyle::SP_FileIcon) : icon);
        button->setIconSize(QSize(38, 38));
        button->setFixedSize(92, 70);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:1px solid transparent;border-radius:12px;color:#ffffff;padding:4px;} QPushButton:hover{background:rgba(255,255,255,0.18);border-color:rgba(255,255,255,0.40);} QPushButton:focus{border:2px solid %1;}").arg(kTeal));
        connect(button, &QPushButton::clicked, this, [command] { QProcess::startDetached(QStringLiteral("bash"), {QStringLiteral("-lc"), command}); });
        m_shortcutButtons.append(button);
    }

    void layoutDesktopShortcuts() {
        if (!m_shortcutsGrid || m_shortcutButtons.isEmpty()) return;
        while (QLayoutItem *item = m_shortcutsGrid->takeAt(0)) delete item;
        const int cellHeight = 80;
        const int availableHeight = qMax(cellHeight, m_desktop->height() - 48);
        const int rows = qMax(1, (availableHeight + m_shortcutsGrid->verticalSpacing()) / (cellHeight + m_shortcutsGrid->verticalSpacing()));
        for (int index = 0; index < m_shortcutButtons.size(); ++index) {
            m_shortcutsGrid->addWidget(m_shortcutButtons.at(index), index % rows, index / rows);
        }
    }

    void buildTaskbar() {
        auto *root = centralWidget();
        m_taskbar = new GlassSurface(root);
        m_taskbar->setFixedHeight(72);
        m_taskbar->setMaterial(m_dark ? QColor(18, 25, 28, 202) : QColor(248, 252, 251, 164), m_dark ? QColor(255, 255, 255, 58) : QColor(255, 255, 255, 185));
        root->layout()->addWidget(m_taskbar);
        auto *layout = new QHBoxLayout(m_taskbar);
        layout->setContentsMargins(16, 8, 16, 8);
        layout->setSpacing(8);

        auto *profile = new QPushButton(currentUserName());
        profile->setIcon(QIcon(userAvatarPath()));
        profile->setIconSize(QSize(34, 34));
        profile->setFixedHeight(52);
        profile->setMinimumWidth(120);
        profile->setCursor(Qt::PointingHandCursor);
        profile->setToolTip(QStringLiteral("Cuenta de usuario · ") + currentUserName());
        profile->setStyleSheet(QStringLiteral("QPushButton{background:rgba(255,255,255,0.14);border:1px solid rgba(255,255,255,0.28);border-radius:14px;padding:5px 10px;text-align:left;color:%1;} QPushButton:hover{background:rgba(255,255,255,0.30);} QPushButton:focus{border:2px solid %2;}").arg(textColor(), accentColor()));
        connect(profile, &QPushButton::clicked, this, [this, profile] { showUserMenu(profile); });
        layout->addWidget(profile);
        layout->addSpacing(6);

        auto *start = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-start.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-start.svg"))), QStringLiteral("Inicio"), [this] { toggleLauncher(); });
        layout->addWidget(start);
        auto *search = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-search.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-search.svg"))), QStringLiteral("Buscar"), [this] { openSearch(); });
        layout->addWidget(search);
        layout->addStretch(1);

        auto *firefox = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-browser.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-browser.svg"))), QStringLiteral("Firefox"), [] { QProcess::startDetached(QStringLiteral("firefox")); });
        auto *dolphin = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-files.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-files.svg"))), QStringLiteral("Dolphin"), [] { QProcess::startDetached(QStringLiteral("dolphin")); });
        auto *vlc = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-media.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-media.svg"))), QStringLiteral("VLC"), [] { QProcess::startDetached(QStringLiteral("vlc")); });
        auto *foundstore = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-foundstore.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-foundstore.svg"))), QStringLiteral("Foundstore"), [] { QProcess::startDetached(QStringLiteral("xdg-open"), {QStringLiteral("https://foundstore.onrender.com")}); });
        layout->addWidget(firefox);
        layout->addWidget(dolphin);
        layout->addWidget(vlc);
        layout->addWidget(foundstore);
        layout->addStretch(1);

        m_system = new QPushButton;
        m_system->setCursor(Qt::PointingHandCursor);
        m_system->setFocusPolicy(Qt::StrongFocus);
        m_system->setFixedSize(48, 50);
        m_system->setIcon(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-wifi.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-wifi.svg"))));
        m_system->setIconSize(QSize(24, 24));
        m_system->setStyleSheet(QStringLiteral("QPushButton{background:rgba(255,255,255,0.17);border:1px solid rgba(255,255,255,0.33);border-radius:17px;color:%1;padding:7px;} QPushButton:hover{background:rgba(255,255,255,0.32);} QPushButton:focus{border:2px solid %2;}").arg(textColor(), accentColor()));
        connect(m_system, &QPushButton::clicked, this, [this] { toggleControlCenter(); });
        layout->addWidget(m_system);

        auto *power = iconButton(QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-power.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-power.svg"))), QStringLiteral("Energía"), [this] { showPowerMenu(); });
        layout->addWidget(power);
    }

    void styleMenu(QMenu *menu) {
        menu->setStyleSheet(QStringLiteral("QMenu{background:rgba(24,32,34,0.96);border:1px solid rgba(255,255,255,0.28);border-radius:12px;padding:6px;color:#e9fff8;} QMenu::item{padding:9px 28px 9px 12px;border-radius:8px;} QMenu::item:selected{background:rgba(22,143,131,0.32);} QMenu::separator{height:1px;background:rgba(255,255,255,0.18);margin:5px 8px;}") );
    }

    void showUserMenu(QWidget *anchor) {
        QMenu menu(anchor);
        styleMenu(&menu);
        QAction *account = menu.addAction(QStringLiteral("Configuración de cuenta"));
        menu.addSeparator();
        QAction *lock = menu.addAction(QStringLiteral("Bloquear"));
        QAction *logout = menu.addAction(QStringLiteral("Cerrar sesión"));
        lock->setEnabled(commandAvailable(QStringLiteral("loginctl")));
        logout->setEnabled(commandAvailable(QStringLiteral("loginctl")));
        connect(account, &QAction::triggered, this, [this] { openSettings(); });
        connect(lock, &QAction::triggered, this, [this] { runPowerCommand(QStringLiteral("loginctl"), {QStringLiteral("lock-session")}); });
        connect(logout, &QAction::triggered, this, [this] { runPowerCommand(QStringLiteral("loginctl"), {QStringLiteral("terminate-session"), qEnvironmentVariable("XDG_SESSION_ID")}); });
        menu.exec(anchor->mapToGlobal(QPoint(0, -menu.sizeHint().height() - 6)));
    }

    void showPowerMenu() {
        QMenu menu(m_taskbar);
        styleMenu(&menu);
        QAction *suspend = menu.addAction(QStringLiteral("Suspender"));
        QAction *restart = menu.addAction(QStringLiteral("Reiniciar"));
        QAction *shutdown = menu.addAction(QStringLiteral("Apagar"));
        menu.addSeparator();
        QAction *cancel = menu.addAction(QStringLiteral("Cancelar"));
        const bool systemctl = commandAvailable(QStringLiteral("systemctl"));
        suspend->setEnabled(systemctl);
        restart->setEnabled(systemctl);
        shutdown->setEnabled(systemctl);
        connect(suspend, &QAction::triggered, this, [this] { runPowerCommand(QStringLiteral("systemctl"), {QStringLiteral("suspend")}); });
        connect(restart, &QAction::triggered, this, [this] { runPowerCommand(QStringLiteral("systemctl"), {QStringLiteral("reboot")}); });
        connect(shutdown, &QAction::triggered, this, [this] { runPowerCommand(QStringLiteral("systemctl"), {QStringLiteral("poweroff")}); });
        Q_UNUSED(cancel);
        const QPoint cursor = QCursor::pos();
        const QRect screen = QGuiApplication::screenAt(cursor) ? QGuiApplication::screenAt(cursor)->availableGeometry() : QGuiApplication::primaryScreen()->availableGeometry();
        const QSize menuSize = menu.sizeHint();
        const int x = qBound(screen.left(), cursor.x() - menuSize.width() + 12, screen.right() - menuSize.width());
        const int y = qBound(screen.top(), cursor.y() - menuSize.height() - 8, screen.bottom() - menuSize.height());
        menu.exec(QPoint(x, y));
    }

    void runPowerCommand(const QString &command, const QStringList &arguments) {
        if (!commandAvailable(command)) {
            QMessageBox::information(this, QStringLiteral("Acción no disponible"), QStringLiteral("El comando de energía no está disponible en este entorno."));
            return;
        }
        QProcess::startDetached(command, arguments);
    }

    void buildLauncher() {
        m_launcher = new GlassSurface(m_desktop);
        m_launcher->setFixedSize(700, 430);
        m_launcher->setMaterial(m_dark ? QColor(20, 28, 31, 224) : QColor(244, 250, 248, 196), m_dark ? QColor(255, 255, 255, 68) : QColor(255, 255, 255, 196));
        auto *layout = new QVBoxLayout(m_launcher);
        layout->setContentsMargins(22, 19, 22, 16);
        layout->setSpacing(9);
        auto *title = new QLabel(QStringLiteral("Inicio · ") + editionValue(QStringLiteral("EDITION_ID"), QStringLiteral("home")));
        title->setStyleSheet(QStringLiteral("font-size:25px;font-weight:700;color:%1;").arg(textColor()));
        layout->addWidget(title);
        m_search = new QLineEdit;
        m_search->setPlaceholderText(QStringLiteral("Buscar aplicaciones y ajustes"));
        m_search->setStyleSheet(QStringLiteral("QLineEdit{background:rgba(255,255,255,0.27);border:1px solid rgba(255,255,255,0.54);border-radius:12px;padding:10px;color:%1;} QLineEdit:focus{border:2px solid %2;}").arg(textColor(), accentColor()));
        layout->addWidget(m_search);

        auto *columns = new QHBoxLayout;
        columns->setSpacing(12);
        auto *applications = new QWidget(m_launcher);
        auto *applicationsLayout = new QVBoxLayout(applications);
        applicationsLayout->setContentsMargins(0, 0, 0, 0);
        auto *applicationsTitle = new QLabel(QStringLiteral("Aplicaciones"));
        applicationsTitle->setStyleSheet(QStringLiteral("font-size:14px;font-weight:600;color:%1;").arg(textColor()));
        applicationsLayout->addWidget(applicationsTitle);
        m_appsList = new ReorderListWidget(applications);
        m_appsList->setIconSize(QSize(34, 34));
        m_appsList->setSpacing(5);
        m_appsList->setDragEnabled(true);
        m_appsList->setAcceptDrops(true);
        m_appsList->setDropIndicatorShown(true);
        m_appsList->setDragDropMode(QAbstractItemView::InternalMove);
        m_appsList->setDefaultDropAction(Qt::MoveAction);
        m_appsList->setStyleSheet(QStringLiteral("QListWidget{background:rgba(255,255,255,0.12);border:1px solid rgba(255,255,255,0.30);border-radius:12px;padding:6px;color:%1;} QListWidget::item{padding:8px;border-radius:8px;} QListWidget::item:hover{background:rgba(255,255,255,0.28);} QListWidget::item:selected{background:rgba(22,143,131,0.24);border:1px solid %2;}").arg(textColor(), accentColor()));
        addLauncherApp(m_appsList, QStringLiteral("Firefox"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-browser.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-browser.svg"))), QStringLiteral("firefox"));
        addLauncherApp(m_appsList, QStringLiteral("Dolphin"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-files.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-files.svg"))), QStringLiteral("dolphin"));
        addLauncherApp(m_appsList, QStringLiteral("VLC"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-media.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-media.svg"))), QStringLiteral("vlc"));
        addLauncherApp(m_appsList, QStringLiteral("Foundstore"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-foundstore.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-foundstore.svg"))), QStringLiteral("xdg-open https://foundstore.onrender.com"));
        addLauncherApp(m_appsList, QStringLiteral("Configuración"), QIcon(assetPath(QStringLiteral("/usr/share/icons/danenone/apps/scalable/danenone-settings.svg"), QStringLiteral("/home/ubuntu/danenone/native-shell/assets/icons/danenone-settings.svg"))), QStringLiteral("__danenone_settings__"));
        restoreListOrder(m_appsList, QStringLiteral("start_apps_order"));
        applicationsLayout->addWidget(m_appsList, 1);
        columns->addWidget(applications, 1);

        auto *widgets = new QWidget(m_launcher);
        auto *widgetsLayout = new QVBoxLayout(widgets);
        widgetsLayout->setContentsMargins(0, 0, 0, 0);
        auto *widgetsTitle = new QLabel(QStringLiteral("Widgets"));
        widgetsTitle->setStyleSheet(QStringLiteral("font-size:14px;font-weight:600;color:%1;").arg(textColor()));
        widgetsLayout->addWidget(widgetsTitle);
        m_widgetList = new ReorderListWidget(widgets);
        m_widgetList->setDragEnabled(true);
        m_widgetList->setAcceptDrops(true);
        m_widgetList->setDropIndicatorShown(true);
        m_widgetList->setDragDropMode(QAbstractItemView::InternalMove);
        m_widgetList->setDefaultDropAction(Qt::MoveAction);
        m_widgetList->setStyleSheet(QStringLiteral("QListWidget{background:rgba(255,255,255,0.12);border:1px solid rgba(255,255,255,0.30);border-radius:12px;padding:6px;color:%1;} QListWidget::item{padding:10px;border-radius:8px;} QListWidget::item:hover{background:rgba(255,255,255,0.28);} QListWidget::item:selected{background:rgba(22,143,131,0.24);border:1px solid %2;}").arg(textColor(), accentColor()));
        m_widgetList->addItems({QStringLiteral("Estado de red\nNo disponible"), QStringLiteral("Energía\nPerfil del sistema"), updatePolicyLabel(), storageStatus()});
        restoreListOrder(m_widgetList, QStringLiteral("start_widgets_order"));
        widgetsLayout->addWidget(m_widgetList, 1);
        columns->addWidget(widgets, 1);
        layout->addLayout(columns, 1);
        auto *hint = new QLabel(QStringLiteral("Arrastra aplicaciones o widgets para reordenarlos · Ctrl+K o Ctrl+Espacio para buscar"));
        hint->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(mutedColor()));
        layout->addWidget(hint);

        connect(m_search, &QLineEdit::textChanged, this, [this](const QString &query) {
            const QString needle = query.trimmed();
            for (int i = 0; i < m_appsList->count(); ++i) {
                QListWidgetItem *item = m_appsList->item(i);
                item->setHidden(!needle.isEmpty() && !item->text().contains(needle, Qt::CaseInsensitive));
            }
        });
        connect(m_appsList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) { launchApplication(item); });
        static_cast<ReorderListWidget *>(m_appsList)->onReordered = [this] { saveListOrder(m_appsList, QStringLiteral("start_apps_order")); };
        static_cast<ReorderListWidget *>(m_widgetList)->onReordered = [this] { saveListOrder(m_widgetList, QStringLiteral("start_widgets_order")); };
        m_launcher->hide();
    }

    void restoreListOrder(QListWidget *list, const QString &key) {
        const QString value = preferenceValue(key);
        if (value.isEmpty()) return;
        const QStringList order = value.split('|', Qt::SkipEmptyParts);
        QList<QListWidgetItem*> items;
        while (list->count() > 0) items.append(list->takeItem(0));
        for (const QString &name : order) {
            for (int i = 0; i < items.size(); ++i) {
                if (items.at(i)->text().section('\n', 0, 0) == name) { list->addItem(items.takeAt(i)); break; }
            }
        }
        for (QListWidgetItem *item : items) list->addItem(item);
    }

    void saveListOrder(QListWidget *list, const QString &key) {
        QStringList names;
        for (int i = 0; i < list->count(); ++i) names.append(list->item(i)->text().section('\n', 0, 0));
        persistPreference(key, names.join('|'));
    }

    void addLauncherApp(QListWidget *list, const QString &name, const QIcon &icon, const QString &command) {
        auto *item = new QListWidgetItem(icon.isNull() ? style()->standardIcon(QStyle::SP_FileIcon) : icon, name, list);
        item->setData(Qt::UserRole, command);
        item->setToolTip(QStringLiteral("Abrir ") + name + QStringLiteral(" · doble clic o Enter"));
    }

    void launchApplication(QListWidgetItem *item) {
        if (!item) return;
        const QString command = item->data(Qt::UserRole).toString();
        if (command == QStringLiteral("__danenone_settings__")) { openSettings(); return; }
        if (!command.isEmpty()) QProcess::startDetached(QStringLiteral("bash"), {QStringLiteral("-lc"), command});
        m_launcher->hide();
    }

    void buildControlCenter() {
        m_control = new GlassSurface(m_desktop);
        m_control->setFixedSize(380, 470);
        m_control->setMaterial(m_dark ? QColor(20, 28, 31, 220) : QColor(244, 250, 248, 188), m_dark ? QColor(255, 255, 255, 62) : QColor(255, 255, 255, 190));
        auto *layout = new QVBoxLayout(m_control);
        layout->setContentsMargins(20, 18, 20, 16);
        layout->setSpacing(7);
        auto *title = new QLabel(QStringLiteral("Centro de control"));
        title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:700;color:%1;").arg(textColor()));
        layout->addWidget(title);
        m_network = addHardwareLabel(layout, QStringLiteral("Red"));
        m_bluetooth = addHardwareLabel(layout, QStringLiteral("Bluetooth"));
        m_sound = addHardwareLabel(layout, QStringLiteral("Sonido"));
        m_battery = addHardwareLabel(layout, QStringLiteral("Batería"));
        m_brightnessSlider = addHardwareSlider(layout, QStringLiteral("Brillo"), m_hardware->brightnessPercent(), [this](int value) { m_hardware->setBrightnessPercent(value); });
        m_volumeSlider = addHardwareSlider(layout, QStringLiteral("Volumen"), m_hardware->volumePercent(), [this](int value) { m_hardware->setVolumePercent(value); });
        m_power = addHardwareLabel(layout, QStringLiteral("Energía"));
        auto *close = new QPushButton(QStringLiteral("Cerrar"));
        close->setStyleSheet(QStringLiteral("QPushButton{background:rgba(255,255,255,0.20);border:1px solid rgba(255,255,255,0.44);border-radius:10px;padding:7px;color:%1;} QPushButton:hover{background:rgba(255,255,255,0.36);} QPushButton:focus{border:2px solid %2;}").arg(textColor(), kTeal));
        connect(close, &QPushButton::clicked, this, [this] { m_control->hide(); });
        layout->addStretch(1);
        layout->addWidget(close);
        m_control->hide();
    }

    QLabel *addHardwareLabel(QVBoxLayout *layout, const QString &title) {
        auto *label = new QLabel(title + QStringLiteral("\nNo disponible"));
        label->setMinimumHeight(40);
        label->setStyleSheet(QStringLiteral("QLabel{background:rgba(255,255,255,0.20);border:1px solid rgba(255,255,255,0.41);border-radius:11px;padding:8px;color:%1;}").arg(textColor()));
        layout->addWidget(label);
        return label;
    }

    QSlider *addHardwareSlider(QVBoxLayout *layout, const QString &title, int value, const std::function<void(int)> &setter) {
        auto *container = new QWidget(m_control);
        auto *row = new QVBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(2);
        auto *label = new QLabel(value >= 0 ? title + QStringLiteral("  ") + QString::number(value) + QStringLiteral(" %") : title + QStringLiteral("  no disponible"), container);
        label->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(mutedColor()));
        auto *slider = new QSlider(Qt::Horizontal, container);
        slider->setRange(0, 100);
        slider->setValue(qMax(0, value));
        slider->setEnabled(value >= 0);
        slider->setStyleSheet(QStringLiteral("QSlider{background:transparent;} QSlider::groove:horizontal{height:6px;background:rgba(255,255,255,0.34);border-radius:3px;} QSlider::sub-page:horizontal{background:%1;border-radius:3px;} QSlider::handle:horizontal{width:16px;height:16px;margin:-5px 0;border-radius:8px;background:#ffffff;border:1px solid %1;}").arg(kTeal));
        connect(slider, &QSlider::valueChanged, this, [label, title, setter](int current) { label->setText(title + QStringLiteral("  ") + QString::number(current) + QStringLiteral(" %")); setter(current); });
        row->addWidget(label);
        row->addWidget(slider);
        layout->addWidget(container);
        return slider;
    }

    void refreshHardware() {
        const QString network = m_hardware->network();
        const QString bluetooth = m_hardware->bluetooth();
        const QString sound = m_hardware->sound();
        const QString battery = m_hardware->battery();
        m_system->setToolTip(network + QStringLiteral("\n") + bluetooth + QStringLiteral("\n") + sound + QStringLiteral("\n") + battery);
        m_network->setText(QStringLiteral("Red\n") + network);
        m_bluetooth->setText(QStringLiteral("Bluetooth\n") + bluetooth);
        m_sound->setText(QStringLiteral("Sonido\n") + sound);
        m_battery->setText(QStringLiteral("Batería\n") + battery);
        m_power->setText(QStringLiteral("Energía\n") + m_hardware->powerProfile());
        if (m_brightnessSlider) { const QSignalBlocker blocker(m_brightnessSlider); const int value = m_hardware->brightnessPercent(); m_brightnessSlider->setEnabled(value >= 0); if (value >= 0) m_brightnessSlider->setValue(value); }
        if (m_volumeSlider) { const QSignalBlocker blocker(m_volumeSlider); const int value = m_hardware->volumePercent(); m_volumeSlider->setEnabled(value >= 0); if (value >= 0) m_volumeSlider->setValue(value); }
    }

    void centerLauncher() {
        m_launcher->move((m_desktop->width() - m_launcher->width()) / 2, qMax(24, m_desktop->height() - m_launcher->height() - 24));
    }

    void placeControl() {
        m_control->move(m_desktop->width() - m_control->width() - 22, m_desktop->height() - m_control->height() - 22);
    }

    void openSettings() {
        SettingsWindow settings(this);
        settings.exec();
    }

    void openSearch() {
        m_control->hide();
        if (!m_launcher->isVisible()) {
            centerLauncher();
            m_launcher->show();
            m_launcher->raise();
        }
        m_search->setFocus();
        m_search->selectAll();
    }

    void toggleLauncher() {
        m_control->hide();
        if (m_launcher->isVisible()) { m_launcher->hide(); return; }
        centerLauncher();
        m_launcher->show();
        m_launcher->raise();
        m_search->setFocus();
    }

    void toggleControlCenter() {
        m_launcher->hide();
        if (m_control->isVisible()) { m_control->hide(); return; }
        placeControl();
        m_control->show();
        m_control->raise();
    }

    bool m_dark = false;
    Hardware *m_hardware = nullptr;
    QTimer *m_refresh = nullptr;
    QWidget *m_desktop = nullptr;
    QLabel *m_wallpaper = nullptr;
    QWidget *m_desktopLayer = nullptr;
    QGridLayout *m_shortcutsGrid = nullptr;
    QList<QPushButton*> m_shortcutButtons;
    QWidget *m_notch = nullptr;
    GlassSurface *m_taskbar = nullptr;
    GlassSurface *m_launcher = nullptr;
    GlassSurface *m_control = nullptr;
    QLineEdit *m_search = nullptr;
    ReorderListWidget *m_appsList = nullptr;
    ReorderListWidget *m_widgetList = nullptr;
    QPushButton *m_system = nullptr;
    QLabel *m_network = nullptr;
    QLabel *m_bluetooth = nullptr;
    QLabel *m_sound = nullptr;
    QLabel *m_battery = nullptr;
    QLabel *m_power = nullptr;
    QSlider *m_brightnessSlider = nullptr;
    QSlider *m_volumeSlider = nullptr;
};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Influent Danenone"));
    ShellWindow shell;
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    shell.setGeometry(screen);
    shell.show();
    if (qEnvironmentVariableIsSet("DANENONE_QT_SHOW_SETTINGS")) QTimer::singleShot(350, &shell, [&shell] { shell.showSettingsForTest(); });
    return app.exec();
}
