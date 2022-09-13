#include "dlgausruestung.h"
#include "ui_dlgausruestung.h"
#include <QKeyEvent>
#include <QMessageBox>
#include "brauhelfer.h"
#include "settings.h"
#include "proxymodel.h"
#include "model/proxymodelsudcolored.h"
#include "model/textdelegate.h"
#include "model/spinboxdelegate.h"
#include "model/doublespinboxdelegate.h"
#include "model/checkboxdelegate.h"
#include "model/comboboxdelegate.h"
#include "model/datedelegate.h"
#include "dialogs/dlgverdampfung.h"
#include "tababstract.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0))
#define qAsConst(x) (x)
#endif

extern Brauhelfer* bh;
extern Settings* gSettings;

QList<QPair<QString, int> > DlgAusruestung::Typname = {
    {tr("Standard"), static_cast<int>(Brauhelfer::AnlageTyp::Standard)},
    {tr("Grainfather G30"), static_cast<int>(Brauhelfer::AnlageTyp::GrainfatherG30)},
    {tr("Grainfather G70"), static_cast<int>(Brauhelfer::AnlageTyp::GrainfatherG70)},
    {tr("Braumeister 10L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister10)},
    {tr("Braumeister 20L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister20)},
    {tr("Braumeister 50L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister50)},
    {tr("Braumeister 200L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister200)},
    {tr("Braumeister 500L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister500)},
    {tr("Braumeister 1000L"), static_cast<int>(Brauhelfer::AnlageTyp::Braumeister1000)},
    {tr("Brauheld Pro 30L"), static_cast<int>(Brauhelfer::AnlageTyp::BrauheldPro30)}
};

DlgAusruestung* DlgAusruestung::Dialog = nullptr;

DlgAusruestung::DlgAusruestung(QWidget *parent) :
    DlgAbstract(staticMetaObject.className(), parent, Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
    ui(new Ui::DlgAusruestung),
    mRow(0)
{
    ui->setupUi(this);
    ui->lblCurrency->setText(QLocale().currencySymbol());

    QPalette palette = ui->tbHelp->palette();
    palette.setBrush(QPalette::Base, palette.brush(QPalette::ToolTipBase));
    palette.setBrush(QPalette::Text, palette.brush(QPalette::ToolTipText));
    ui->tbHelp->setPalette(palette);

    TableView *table = ui->tableViewAnlagen;
    ProxyModel *model = new ProxyModel(this);
    model->setSourceModel(bh->modelAusruestung());
    model->setHeaderData(ModelAusruestung::ColName, Qt::Horizontal, tr("Anlage"));
    model->setHeaderData(ModelAusruestung::ColTyp, Qt::Horizontal, tr("Typ"));
    model->setHeaderData(ModelAusruestung::ColVermoegen, Qt::Horizontal, tr("Vermögen [l]"));
    model->setHeaderData(ModelAusruestung::ColAnzahlSude, Qt::Horizontal, tr("Anzahl Sude"));
    model->setHeaderData(ModelAusruestung::ColAnzahlGebrauteSude, Qt::Horizontal, tr("Anzahl gebraute Sude"));
    table->setModel(model);
    table->appendCol({ModelAusruestung::ColName, true, false, -1, nullptr});
    table->appendCol({ModelAusruestung::ColTyp, true, true, 100, new ComboBoxDelegate(Typname, table)});
    table->appendCol({ModelAusruestung::ColVermoegen, true, true, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelAusruestung::ColAnzahlSude, true, true, 100, new SpinBoxDelegate(table)});
    table->appendCol({ModelAusruestung::ColAnzahlGebrauteSude, true, true, 100, new SpinBoxDelegate(table)});
    table->build();
    table->setDefaultContextMenu();

    table = ui->tableViewGeraete;
    model = new ProxyModel(this);
    model->setSourceModel(bh->modelGeraete());
    model->setHeaderData(ModelGeraete::ColBezeichnung, Qt::Horizontal, tr("Bezeichnung"));
    model->setFilterKeyColumn(ModelGeraete::ColAusruestungAnlagenID);
    table->setModel(model);
    table->appendCol({ModelGeraete::ColBezeichnung, true, false, -1, nullptr});
    table->build();
    table->setDefaultContextMenu();

    table = ui->tableViewSude;
    model = new ProxyModelSudColored(this);
    model->setSourceModel(bh->modelSud());
    model->setFilterKeyColumn(ModelSud::ColAnlage);
    table->setModel(model);
    table->appendCol({ModelSud::ColSudname, true, false, 200, new TextDelegate(true, Qt::AlignLeft | Qt::AlignVCenter, table)});
    table->appendCol({ModelSud::ColSudnummer, true, true, 80, new SpinBoxDelegate(table)});
    table->appendCol({ModelSud::ColKategorie, true, true, 100, new TextDelegate(false, Qt::AlignCenter, table)});
    table->appendCol({ModelSud::ColBraudatum, true, false, 100, new DateDelegate(false, true, table)});
    table->appendCol({ModelSud::Colerg_EffektiveAusbeute, true, false, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelSud::ColVerdampfungsrateIst, true, false, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelSud::ColAusbeuteIgnorieren, true, false, 150, new CheckBoxDelegate(table)});
    table->build();
    table->setDefaultContextMenu();

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter->setStretchFactor(2, 0);
    ui->splitter->setSizes({60, 60, 60});

    modulesChanged(Settings::ModuleAlle);
    connect(gSettings, SIGNAL(modulesChanged(Settings::Modules)), this, SLOT(modulesChanged(Settings::Modules)));

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(focusChanged(QWidget*,QWidget*)));
    connect(bh->sud(), SIGNAL(loadedChanged()), this, SLOT(sudLoaded()));
    connect(bh->modelSud(), SIGNAL(modified()), this, SLOT(updateDurchschnitt()));
    connect(bh->modelAusruestung(), SIGNAL(modified()), this, SLOT(updateValues()));
    connect(ui->tableViewAnlagen->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(anlage_selectionChanged()));

    connect(ui->wdgBemerkung, &WdgBemerkung::changed, this, [this](const QString& html){setData(ModelAusruestung::ColBemerkung, html);});
}

DlgAusruestung::~DlgAusruestung()
{
    delete ui;
}

void DlgAusruestung::saveSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->setValue("tableStateAnlagen", ui->tableViewAnlagen->horizontalHeader()->saveState());
    gSettings->setValue("tableStateGeraete", ui->tableViewGeraete->horizontalHeader()->saveState());
    gSettings->setValue("tableStateSude", ui->tableViewSude->horizontalHeader()->saveState());
    gSettings->setValue("splitterStateMain", ui->splitter->saveState());
    gSettings->setValue("AnzahlDurchschnitt", ui->sliderAusbeuteSude->value());
    gSettings->endGroup();
}

void DlgAusruestung::loadSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    ui->tableViewAnlagen->restoreState(gSettings->value("tableStateAnlagen").toByteArray());
    ui->tableViewGeraete->restoreState(gSettings->value("tableStateGeraete").toByteArray());
    ui->tableViewSude->restoreState(gSettings->value("tableStateSude").toByteArray());
    ui->splitter->restoreState(gSettings->value("splitterStateMain").toByteArray());
    ui->sliderAusbeuteSude->setValue(gSettings->value("AnzahlDurchschnitt").toInt());
    gSettings->endGroup();
}

void DlgAusruestung::restoreView()
{
    DlgAbstract::restoreView(staticMetaObject.className());
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->remove("tableStateAnlagen");
    gSettings->remove("tableStateGeraete");
    gSettings->remove("tableStateSude");
    gSettings->remove("splitterStateMain");
    gSettings->endGroup();
}

void DlgAusruestung::modulesChanged(Settings::Modules modules)
{
    if (modules.testFlag(Settings::ModulePreiskalkulation))
    {
        TabAbstract::setVisibleModule(Settings::ModulePreiskalkulation,
                                      {ui->tbKosten,
                                       ui->lblKosten,
                                       ui->lblCurrency});
    }
}

void DlgAusruestung::showEvent(QShowEvent *event)
{
    DlgAbstract::showEvent(event);
    sudLoaded();
}

void DlgAusruestung::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (ui->tableViewAnlagen->hasFocus())
    {
        switch (event->key())
        {
        case Qt::Key::Key_Delete:
            on_btnAnlageLoeschen_clicked();
            break;
        case Qt::Key::Key_Insert:
            on_btnNeueAnlage_clicked();
            break;
        }
    }
    else if (ui->tableViewGeraete->hasFocus())
    {
        switch (event->key())
        {
        case Qt::Key::Key_Delete:
            on_btnGeraetLoeschen_clicked();
            break;
        case Qt::Key::Key_Insert:
            on_btnNeuesGeraet_clicked();
            break;
        }
    }
}

void DlgAusruestung::focusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)
    if (now && isAncestorOf(now) && now != ui->tbHelp && !qobject_cast<QSplitter*>(now))
        ui->tbHelp->setHtml(now->toolTip());
}

void DlgAusruestung::sudLoaded()
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    int row = mRow;
    if (bh->sud()->isLoaded())
    {
        row = model->getRowWithValue(ModelAusruestung::ColName, bh->sud()->getAnlage());
        if (row < 0)
            row = mRow;
    }
    ui->tableViewAnlagen->setCurrentIndex(model->index(row, ModelAusruestung::ColName));
}

void DlgAusruestung::anlage_selectionChanged()
{
    QRegularExpression regExpId;
    QRegularExpression regExpId2;
    QModelIndexList selectedRows = ui->tableViewAnlagen->selectionModel()->selectedRows();
    if (selectedRows.count() > 0)
    {
        mRow = selectedRows[0].row();
        ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
        QString anlage = model->data(model->index(mRow, ModelAusruestung::ColName)).toString();
        regExpId = QRegularExpression(QString("^%1$").arg(QRegularExpression::escape(anlage)));
        regExpId2 = QRegularExpression(QString("^%1$").arg(model->data(model->index(mRow, ModelAusruestung::ColID)).toInt()));
        ui->scrollArea->setEnabled(true);
    }
    else
    {
        regExpId = QRegularExpression(QString("--dummy--"));
        regExpId2 = QRegularExpression(QString("--dummy--"));
        ui->scrollArea->setEnabled(false);
    }
    static_cast<ProxyModel*>(ui->tableViewGeraete->model())->setFilterRegularExpression(regExpId2);
    static_cast<ProxyModel*>(ui->tableViewSude->model())->setFilterRegularExpression(regExpId);
    ui->sliderAusbeuteSude->setMaximum(9999);
    if (ui->sliderAusbeuteSude->value() == 0)
        ui->sliderAusbeuteSude->setValue(9999);
    updateValues();
    updateDurchschnitt();
}

void DlgAusruestung::on_btnNeueAnlage_clicked()
{
    QMap<int, QVariant> values({{ModelAusruestung::ColName, tr("Neue Brauanlage")}});
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    int row = model->append(values);
    if (row >= 0)
    {
        const QModelIndex index = model->index(row, ModelAusruestung::ColName);
        ui->tableViewAnlagen->setCurrentIndex(index);
        ui->tableViewAnlagen->scrollTo(ui->tableViewAnlagen->currentIndex());
        ui->tableViewAnlagen->edit(ui->tableViewAnlagen->currentIndex());
    }
}

void DlgAusruestung::on_btnAnlageLoeschen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableViewAnlagen->model());
    QModelIndexList indices = ui->tableViewAnlagen->selectionModel()->selectedRows();
    std::sort(indices.begin(), indices.end(), [](const QModelIndex & a, const QModelIndex & b){ return a.row() > b.row(); });
    for (const QModelIndex& index : qAsConst(indices))
    {
        QString name = model->data(index.row(), ModelAusruestung::ColName).toString();
        int ret = QMessageBox::question(this, tr("Brauanlage löschen?"),
                                        tr("Soll die Brauanlage \"%1\" gelöscht werden?").arg(name));
        if (ret == QMessageBox::Yes)
        {
            ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
            int row = model->getRowWithValue(ModelAusruestung::ColName, name);
            model->removeRow(row);
        }
    }
}

void DlgAusruestung::on_btnNeuesGeraet_clicked()
{
    QModelIndexList selected = ui->tableViewAnlagen->selectionModel()->selectedRows();
    if (selected.count() > 0)
    {
        QMap<int, QVariant> values({{ModelGeraete::ColAusruestungAnlagenID, data(ModelAusruestung::ColID)},
                                    {ModelGeraete::ColBezeichnung, tr("Neues Gerät")}});
        ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewGeraete->model());
        int row = model->append(values);
        if (row >= 0)
        {
            const QModelIndex index = model->index(row, ModelGeraete::ColBezeichnung);
            ui->tableViewGeraete->setCurrentIndex(index);
            ui->tableViewGeraete->scrollTo(ui->tableViewGeraete->currentIndex());
            ui->tableViewGeraete->edit(ui->tableViewGeraete->currentIndex());
        }
    }
}

void DlgAusruestung::on_btnGeraetLoeschen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableViewGeraete->model());
    QModelIndexList indices = ui->tableViewGeraete->selectionModel()->selectedRows();
    std::sort(indices.begin(), indices.end(), [](const QModelIndex & a, const QModelIndex & b){ return a.row() > b.row(); });
    for (const QModelIndex& index : qAsConst(indices))
    {
        QString name = model->data(index.row(), ModelGeraete::ColBezeichnung).toString();
        int ret = QMessageBox::question(this, tr("Gerät löschen?"),
                                        tr("Soll das Gerät \"%1\" gelöscht werden?").arg(name));
        if (ret == QMessageBox::Yes)
        {
            int row = model->getRowWithValue(ModelGeraete::ColBezeichnung, name);
            model->removeRow(row);
        }
    }
}

QVariant DlgAusruestung::data(int col) const
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    return model->data(mRow, col);
}

bool DlgAusruestung::setData(int col, const QVariant &value)
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    return model->setData(mRow, col, value);
}

void DlgAusruestung::updateValues()
{
    ui->lblAnlage->setText(data(ModelAusruestung::ColName).toString());
    if (!ui->tbAusbeute->hasFocus())
        ui->tbAusbeute->setValue(data(ModelAusruestung::ColSudhausausbeute).toDouble());
    if (!ui->tbVerdampfung->hasFocus())
        ui->tbVerdampfung->setValue(data(ModelAusruestung::ColVerdampfungsrate).toDouble());
    if (!ui->tbKorrekturNachguss->hasFocus())
        ui->tbKorrekturNachguss->setValue(data(ModelAusruestung::ColKorrekturWasser).toDouble());
    if (!ui->tbKorrekturFarbe->hasFocus())
        ui->tbKorrekturFarbe->setValue(data(ModelAusruestung::ColKorrekturFarbe).toInt());
    if (!ui->tbKorrekturSollmenge->hasFocus())
        ui->tbKorrekturSollmenge->setValue(data(ModelAusruestung::ColKorrekturMenge).toDouble());
    if (!ui->tbKosten->hasFocus())
        ui->tbKosten->setValue(data(ModelAusruestung::ColKosten).toDouble());
    if (!ui->tbMaischebottichHoehe->hasFocus())
        ui->tbMaischebottichHoehe->setValue(data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble());
    if (!ui->tbMaischebottichDurchmesser->hasFocus())
        ui->tbMaischebottichDurchmesser->setValue(data(ModelAusruestung::ColMaischebottich_Durchmesser).toDouble());
    ui->tbMaischebottichMaxFuellhoehe->setMaximum(data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble());
    if (!ui->tbMaischebottichMaxFuellhoehe->hasFocus())
        ui->tbMaischebottichMaxFuellhoehe->setValue(data(ModelAusruestung::ColMaischebottich_MaxFuellhoehe).toDouble());
    if (!ui->tbSudpfanneHoehe->hasFocus())
        ui->tbSudpfanneHoehe->setValue(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    if (!ui->tbSudpfanneDurchmesser->hasFocus())
        ui->tbSudpfanneDurchmesser->setValue(data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble());
    ui->tbSudpfanneMaxFuellhoehe->setMaximum(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    if (!ui->tbSudpfanneMaxFuellhoehe->hasFocus())
        ui->tbSudpfanneMaxFuellhoehe->setValue(data(ModelAusruestung::ColSudpfanne_MaxFuellhoehe).toDouble());
    ui->tbMaischenVolumen->setValue(data(ModelAusruestung::ColMaischebottich_Volumen).toDouble());
    ui->tbMaischenMaxNutzvolumen->setValue(data(ModelAusruestung::ColMaischebottich_MaxFuellvolumen).toDouble());
    ui->tbSudpfanneVolumen->setValue(data(ModelAusruestung::ColSudpfanne_Volumen).toDouble());
    ui->tbSudpfanneMaxNutzvolumen->setValue(data(ModelAusruestung::ColSudpfanne_MaxFuellvolumen).toDouble());
    ui->wdgBemerkung->setHtml(data(ModelAusruestung::ColBemerkung).toString());
}

void DlgAusruestung::updateDurchschnitt()
{
    QString anlage = data(ModelAusruestung::ColName).toString();
    ProxyModelSud modelSud;
    modelSud.setSourceModel(bh->modelSud());
    QRegularExpression regExp(QString("^%1$").arg(QRegularExpression::escape(anlage)));
    modelSud.setFilterKeyColumn(ModelSud::ColAnlage);
    modelSud.setFilterRegularExpression(regExp);
    modelSud.setFilterStatus(ProxyModelSud::Gebraut | ProxyModelSud::Abgefuellt | ProxyModelSud::Verbraucht);
    modelSud.sort(ModelSud::ColBraudatum, Qt::DescendingOrder);
    int nAusbeute = 0;
    int nVerdampfung = 0;
    int N = 0;
    double ausbeute = 0.0;
    double verdampfung = 0.0;
    for (int i = 0; i < modelSud.rowCount(); ++i)
    {
        if (!modelSud.index(i, ModelSud::ColAusbeuteIgnorieren).data().toBool())
        {
            if (N < ui->sliderAusbeuteSude->value())
            {
                double val = modelSud.index(i, ModelSud::Colerg_EffektiveAusbeute).data().toDouble();
                if (val > 0)
                {
                    ausbeute += val;
                    nAusbeute++;
                }
                val = modelSud.index(i, ModelSud::ColVerdampfungsrateIst).data().toDouble();
                if (val > 0)
                {
                    verdampfung += val;
                    ++nVerdampfung;
                }
            }
            ++N;
        }
    }
    if (nAusbeute > 0)
        ausbeute /= nAusbeute;
    if (nVerdampfung > 0)
        verdampfung /= nVerdampfung;
    ui->sliderAusbeuteSude->setMaximum(N);
    ui->sliderAusbeuteSude->setEnabled(N > 1);
    ui->tbAusbeuteMittel->setValue(ausbeute);
    ui->tbVerdampfungMittel->setValue(verdampfung);
    ui->tbAusbeuteSude->setValue(ui->sliderAusbeuteSude->value());
}

void DlgAusruestung::on_btnVerdampfungsrate_clicked()
{
    WidgetDecorator::suspendValueChanged(true);
    DlgVerdampfung dlg;
    dlg.setDurchmesser(data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble());
    dlg.setHoehe(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    if (bh->sud()->isLoaded())
    {
        dlg.setKochdauer(bh->sud()->getKochdauer());
        dlg.setMenge1(BierCalc::volumenWasser(20, 100, bh->sud()->getWuerzemengeKochbeginn()));
        dlg.setMenge2(BierCalc::volumenWasser(20, 100, bh->sud()->getWuerzemengeVorHopfenseihen()));
    }
    else
    {
        dlg.setKochdauer(90);
        dlg.setMenge1(20);
        dlg.setMenge2(18);
    }
    int dlgRet = dlg.exec();
    WidgetDecorator::suspendValueChanged(false);
    if (dlgRet == QDialog::Accepted)
    {
        setData(ModelAusruestung::ColVerdampfungsrate, dlg.getVerdampfungsrate());
    }
}

void DlgAusruestung::on_tbAusbeute_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudhausausbeute).toDouble();
    if (prevValue != ui->tbAusbeute->value())
        setData(ModelAusruestung::ColSudhausausbeute, ui->tbAusbeute->value());
}

void DlgAusruestung::on_btnAusbeuteMittel_clicked()
{
    double prevValue = data(ModelAusruestung::ColSudhausausbeute).toDouble();
    if (prevValue != ui->tbAusbeuteMittel->value())
        setData(ModelAusruestung::ColSudhausausbeute, ui->tbAusbeuteMittel->value());
}

void DlgAusruestung::on_tbVerdampfung_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColVerdampfungsrate).toDouble();
    if (prevValue != ui->tbVerdampfung->value())
        setData(ModelAusruestung::ColVerdampfungsrate, ui->tbVerdampfung->value());
}

void DlgAusruestung::on_btnVerdampfungMittel_clicked()
{
    double prevValue = data(ModelAusruestung::ColVerdampfungsrate).toDouble();
    if (prevValue != ui->tbVerdampfungMittel->value())
        setData(ModelAusruestung::ColVerdampfungsrate, ui->tbVerdampfungMittel->value());
}

void DlgAusruestung::on_sliderAusbeuteSude_valueChanged(int)
{
    updateDurchschnitt();
}

void DlgAusruestung::on_tbKorrekturNachguss_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturWasser).toDouble();
    if (prevValue != ui->tbKorrekturNachguss->value())
        setData(ModelAusruestung::ColKorrekturWasser, ui->tbKorrekturNachguss->value());
}

void DlgAusruestung::on_tbKorrekturFarbe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturFarbe).toDouble();
    if (prevValue != ui->tbKorrekturFarbe->value())
        setData(ModelAusruestung::ColKorrekturFarbe, ui->tbKorrekturFarbe->value());
}

void DlgAusruestung::on_tbKorrekturSollmenge_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturMenge).toDouble();
    if (prevValue != ui->tbKorrekturSollmenge->value())
        setData(ModelAusruestung::ColKorrekturMenge, ui->tbKorrekturSollmenge->value());
}

void DlgAusruestung::on_tbKosten_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKosten).toDouble();
    if (prevValue != ui->tbKosten->value())
        setData(ModelAusruestung::ColKosten, ui->tbKosten->value());
}

void DlgAusruestung::on_tbMaischebottichHoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble();
    if (prevValue != ui->tbMaischebottichHoehe->value())
        setData(ModelAusruestung::ColMaischebottich_Hoehe, ui->tbMaischebottichHoehe->value());
}

void DlgAusruestung::on_tbMaischebottichDurchmesser_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_Durchmesser).toDouble();
    if (prevValue != ui->tbMaischebottichDurchmesser->value())
        setData(ModelAusruestung::ColMaischebottich_Durchmesser, ui->tbMaischebottichDurchmesser->value());
}

void DlgAusruestung::on_tbMaischebottichMaxFuellhoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_MaxFuellhoehe).toDouble();
    if (prevValue != ui->tbMaischebottichMaxFuellhoehe->value())
        setData(ModelAusruestung::ColMaischebottich_MaxFuellhoehe, ui->tbMaischebottichMaxFuellhoehe->value());
}

void DlgAusruestung::on_tbSudpfanneHoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble();
    if (prevValue != ui->tbSudpfanneHoehe->value())
        setData(ModelAusruestung::ColSudpfanne_Hoehe, ui->tbSudpfanneHoehe->value());
}

void DlgAusruestung::on_tbSudpfanneDurchmesser_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble();
    if (prevValue != ui->tbSudpfanneDurchmesser->value())
        setData(ModelAusruestung::ColSudpfanne_Durchmesser, ui->tbSudpfanneDurchmesser->value());
}

void DlgAusruestung::on_tbSudpfanneMaxFuellhoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_MaxFuellhoehe).toDouble();
    if (prevValue != ui->tbSudpfanneMaxFuellhoehe->value())
        setData(ModelAusruestung::ColSudpfanne_MaxFuellhoehe, ui->tbSudpfanneMaxFuellhoehe->value());
}