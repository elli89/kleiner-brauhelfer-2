#ifndef DLG_AUSRUESTUNG_H
#define DLG_AUSRUESTUNG_H

#include "dlgabstract.h"
#include <QItemSelection>

namespace Ui {
class DlgAusruestung;
}

class DlgAusruestung : public DlgAbstract
{
    Q_OBJECT

public:
    static QList<QPair<QString, int> > Typname;

    static DlgAusruestung *Dialog;

public:
    explicit DlgAusruestung(QWidget *parent = nullptr);
    virtual ~DlgAusruestung() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;
    void restoreView() Q_DECL_OVERRIDE;

private slots:
    void focusChanged(QWidget *old, QWidget *now);
    void sudLoaded();
    void anlage_selectionChanged();
    void on_btnNeueAnlage_clicked();
    void on_btnAnlageLoeschen_clicked();
    void on_btnNeuesGeraet_clicked();
    void on_btnGeraetLoeschen_clicked();
    void updateValues();
    void updateDurchschnitt();
    void on_btnAusbeuteMittel_clicked();
    void on_btnVerdampfungsrate_clicked();
    void on_btnVerdampfungMittel_clicked();
    void on_tbAusbeute_editingFinished();
    void on_sliderAusbeuteSude_valueChanged(int value);
    void on_tbVerdampfung_editingFinished();
    void on_tbKorrekturNachguss_editingFinished();
    void on_tbKorrekturFarbe_editingFinished();
    void on_tbKorrekturSollmenge_editingFinished();
    void on_tbKosten_editingFinished();
    void on_tbMaischebottichHoehe_editingFinished();
    void on_tbMaischebottichDurchmesser_editingFinished();
    void on_tbMaischebottichMaxFuellhoehe_editingFinished();
    void on_tbSudpfanneHoehe_editingFinished();
    void on_tbSudpfanneDurchmesser_editingFinished();
    void on_tbSudpfanneMaxFuellhoehe_editingFinished();

private:
    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    QVariant data(int col) const;
    bool setData(int col, const QVariant &value);

private:
    Ui::DlgAusruestung *ui;
    int mRow;
    QByteArray mDefaultSplitterState;
    QByteArray mDefaultSplitterLeftState;
    QByteArray mDefaultSplitterHelpState;
};

#endif // DLG_AUSRUESTUNG_H
