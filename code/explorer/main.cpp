#include "mainwindow.h"
#include <QApplication>

#include "core/app.h"

using namespace Iridium;

class ExplorerApplication final : public Application
{
public:
    int Run(int argc, char** argv) override;

    StringView GetName() override;
};

// https://ethanschoonover.com/solarized/
namespace Solarized
{
    static const QColor Base03 {0, 43, 54};
    static const QColor Base02 {7, 54, 66};
    static const QColor Base01 {88, 110, 117};
    static const QColor Base00 {101, 123, 131};
    static const QColor Base0 {131, 148, 150};
    static const QColor Base1 {147, 161, 161};
    static const QColor Base2 {238, 232, 213};
    static const QColor Base3 {253, 246, 227};

    static const QColor Yellow {181, 137, 0};
    static const QColor Orange {203, 75, 22};
    static const QColor Red {220, 50, 47};
    static const QColor Magenta {211, 54, 130};
    static const QColor Violet {108, 113, 196};
    static const QColor Blue {38, 139, 210};
    static const QColor Cyan {42, 161, 152};
    static const QColor Green {133, 153, 0};

    static const QColor BaseDark[8] {Base03, Base02, Base01, Base00, Base0, Base1, Base2, Base3};
    static const QColor BaseLight[8] {Base3, Base2, Base1, Base0, Base00, Base01, Base02, Base03};
} // namespace Solarized

static QPalette GetSolarizedPalette(bool dark)
{
    QPalette p;

    const QColor* base = dark ? Solarized::BaseDark : Solarized::BaseLight;

    p.setColor(QPalette::Base, base[0]);
    p.setColor(QPalette::Window, base[0]);
    p.setColor(QPalette::AlternateBase, base[1]);
    p.setColor(QPalette::Highlight, base[2]);

    p.setColor(QPalette::Text, base[5]);
    p.setColor(QPalette::WindowText, base[5]);
    p.setColor(QPalette::BrightText, base[5]);
    p.setColor(QPalette::ButtonText, base[5]);
    p.setColor(QPalette::PlaceholderText, base[3]);
    p.setColor(QPalette::HighlightedText, base[6]);

    p.setColor(QPalette::Dark, base[0]);
    p.setColor(QPalette::Mid, base[1]);
    p.setColor(QPalette::Button, base[1]);
    p.setColor(QPalette::Midlight, base[2]);
    p.setColor(QPalette::Light, base[3]);

    p.setColor(QPalette::Shadow, Qt::black);

    p.setColor(QPalette::Link, Solarized::Blue);
    p.setColor(QPalette::LinkVisited, Solarized::Red);

    p.setColor(QPalette::NoRole, Solarized::Magenta);

    p.setColor(QPalette::ToolTipBase, base[1]);
    p.setColor(QPalette::ToolTipText, Solarized::Violet);

    return p;
}

int ExplorerApplication::Run(int argc, char** argv)
{
    QApplication a(argc, argv);

    a.setStyle("Fusion");

    QFont f("Consolas");
    f.setPointSize(11);
    a.setFont(f);

    a.setPalette(GetSolarizedPalette(true));

    MainWindow w;
    w.show();

    return a.exec();
}

StringView ExplorerApplication::GetName()
{
    return "Iridium Explorer";
}

Ptr<Application> Iridium::CreateApplication()
{
    return MakeUnique<ExplorerApplication>();
}
