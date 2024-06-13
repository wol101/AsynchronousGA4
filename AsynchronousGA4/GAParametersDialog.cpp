#include "GAParametersDialog.h"
#include "ui_GAParametersDialog.h"

#include <QSettings>
#include <QFile>
#include <QDomDocument>

#include <vector>
#include <string>

GAParametersDialog::GAParametersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GAParametersDialog)
{
    ui->setupUi(this);

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &GAParametersDialog::reject);
    connect(ui->pushButtonOK, &QPushButton::clicked, this, &GAParametersDialog::accept);

    // force a button enable check when tbne combo boxes are altered (since they can have invalid values
    QList<QComboBox *> listQComboBox = this->findChildren<QComboBox *>(QString(), Qt::FindChildrenRecursively);
    for (auto &&it : listQComboBox) { connect(it, &QComboBox::currentIndexChanged, this, &GAParametersDialog::activateButtons); }
    activateButtons();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    restoreGeometry(settings.value("GAParametersDialogGeometry").toByteArray());
}

GAParametersDialog::~GAParametersDialog()
{
    delete ui;
}

void GAParametersDialog::closeEvent(QCloseEvent *event)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    settings.setValue("GAParametersDialogGeometry", saveGeometry());
    settings.sync();
    QDialog::closeEvent(event);
}

QString GAParametersDialog::editorText() const
{
    QString xmlString;
    QDomDocument doc("AsynchronousGA_GAParameters_Document_0.1");
    QDomProcessingInstruction  pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(pi);
    QDomElement root = doc.createElement("ASYNCHRONOUSGA");
    doc.appendChild(root);

    QDomElement dataItemsElement = doc.createElement("GAPARAMETERS");
    root.appendChild(dataItemsElement);

    dataItemsElement.setAttribute("crossoverType", QString("%1").arg(ui->comboBoxCrossoverType->currentText()));
    dataItemsElement.setAttribute("parentSelection", QString("%1").arg(ui->comboBoxParentSelection->currentText()));
    dataItemsElement.setAttribute("resizeControl", QString("%1").arg(ui->comboBoxResizeControl->currentText()));

    dataItemsElement.setAttribute("bounceMutation", QString("%1").arg(ui->checkBoxBounceMutation->isChecked()));
    dataItemsElement.setAttribute("circularMutation", QString("%1").arg(ui->checkBoxCircularMutation->isChecked()));
    dataItemsElement.setAttribute("multipleGaussian", QString("%1").arg(ui->checkBoxMultipleGaussian->isChecked()));
    dataItemsElement.setAttribute("onlyKeepBestGenome", QString("%1").arg(ui->checkBoxOnlyKeepBestGenome->isChecked()));
    dataItemsElement.setAttribute("onlyKeepBestPopulation", QString("%1").arg(ui->checkBoxOnlyKeepBestPopulation->isChecked()));
    dataItemsElement.setAttribute("randomiseModel", QString("%1").arg(ui->checkBoxRandomiseModel->isChecked()));
    dataItemsElement.setAttribute("minimizeScore", QString("%1").arg(ui->checkBoxMinimizeScore->isChecked()));

    dataItemsElement.setAttribute("crossoverChance", QString("%1").arg(ui->doubleSpinBoxCrossoverChance->value()));
    dataItemsElement.setAttribute("duplicationMutationChance", QString("%1").arg(ui->doubleSpinBoxDuplicationMutationChance->value()));
    dataItemsElement.setAttribute("frameShiftMutationChance", QString("%1").arg(ui->doubleSpinBoxFrameShiftMutationChance->value()));
    dataItemsElement.setAttribute("gamma", QString("%1").arg(ui->doubleSpinBoxGamma->value()));
    dataItemsElement.setAttribute("gaussianMutationChance", QString("%1").arg(ui->doubleSpinBoxGaussianMutationChance->value()));
    dataItemsElement.setAttribute("improvementThreshold", QString("%1").arg(ui->doubleSpinBoxImprovementThreshold->value()));
    dataItemsElement.setAttribute("watchDogTimerLimit", QString("%1").arg(ui->doubleSpinBoxWatchDogTimerLimit->value()));

    dataItemsElement.setAttribute("genomeLength", QString("%1").arg(ui->spinBoxGenomeLength->value()));
    dataItemsElement.setAttribute("improvementReproductions", QString("%1").arg(ui->spinBoxImprovementReproductions->value()));
    dataItemsElement.setAttribute("maxReproductions", QString("%1").arg(ui->spinBoxMaxReproductions->value()));
    dataItemsElement.setAttribute("outputPopulationSize", QString("%1").arg(ui->spinBoxOutputPopulationSize->value()));
    dataItemsElement.setAttribute("outputStatsEvery", QString("%1").arg(ui->spinBoxOutputStatsEvery->value()));
    dataItemsElement.setAttribute("parentsToKeep", QString("%1").arg(ui->spinBoxParentsToKeep->value()));
    dataItemsElement.setAttribute("populationSize", QString("%1").arg(ui->spinBoxPopulationSize->value()));
    dataItemsElement.setAttribute("saveBestEvery", QString("%1").arg(ui->spinBoxSaveBestEvery->value()));
    dataItemsElement.setAttribute("savePopEvery", QString("%1").arg(ui->spinBoxSavePopEvery->value()));

    // and now the actual xml doc
    xmlString = doc.toString();
    return xmlString;
}

void GAParametersDialog::setEditorText(const QString &newEditorText)
{
    QDomDocument doc;
    QDomElement docElem = doc.documentElement();
    if (docElem.tagName() != "ASYNCHRONOUSGA") return;
    QDomNode n = docElem.firstChild();
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull())
        {
            if (e.tagName() == "GAPARAMETERS")
            {
                static const std::vector<std::string> crossoverType = {"OnePoint", "Average"};
                static const std::vector<std::string> parentSelection = {"UniformSelection", "RankBasedSelection", "SqrtBasedSelection", "GammaBasedSelection"};
                static const std::vector<std::string> resizeControl = {"RandomiseResize", "MutateResize"};

                std::string stringV;
                int index;
                stringV = e.attribute("crossoverType").toStdString();
                index = -1;
                for (size_t i = 0; i < crossoverType.size(); i++)
                {
                    ui->comboBoxCrossoverType->addItem(QString::fromStdString(crossoverType[i]));
                    if (crossoverType[i] == stringV) index = int(i);
                }
                ui->comboBoxCrossoverType->setCurrentIndex(index);

                stringV = e.attribute("resizeControl").toStdString();
                index = -1;
                for (size_t i = 0; i < resizeControl.size(); i++)
                {
                    ui->comboBoxResizeControl->addItem(QString::fromStdString(resizeControl[i]));
                    if (resizeControl[i] == stringV) index = int(i);
                }
                ui->comboBoxResizeControl->setCurrentIndex(index);

                stringV = e.attribute("parentSelection").toStdString();
                index = -1;
                for (size_t i = 0; i < parentSelection.size(); i++)
                {
                    ui->comboBoxParentSelection->addItem(QString::fromStdString(parentSelection[i]));
                    if (parentSelection[i] == stringV) index = int(i);
                }
                ui->comboBoxParentSelection->setCurrentIndex(index);

                bool boolV;
                boolV = e.attribute("bounceMutation").toInt();
                ui->checkBoxBounceMutation->setChecked(boolV);
                boolV = e.attribute("circularMutation").toInt();
                ui->checkBoxCircularMutation->setChecked(boolV);
                boolV = e.attribute("multipleGaussian").toInt();
                ui->checkBoxMultipleGaussian->setChecked(boolV);
                boolV = e.attribute("onlyKeepBestGenome").toInt();
                ui->checkBoxOnlyKeepBestGenome->setChecked(boolV);
                boolV = e.attribute("onlyKeepBestPopulation").toInt();
                ui->checkBoxOnlyKeepBestPopulation->setChecked(boolV);
                boolV = e.attribute("randomiseModel").toInt();
                ui->checkBoxRandomiseModel->setChecked(boolV);
                boolV = e.attribute("minimizeScore").toInt();
                ui->checkBoxMinimizeScore->setChecked(boolV);

                double doubleV;
                doubleV = e.attribute("crossoverChance").toDouble();
                ui->doubleSpinBoxCrossoverChance->setValue(doubleV);
                doubleV = e.attribute("duplicationMutationChance").toDouble();
                ui->doubleSpinBoxDuplicationMutationChance->setValue(doubleV);
                doubleV = e.attribute("frameShiftMutationChance").toDouble();
                ui->doubleSpinBoxFrameShiftMutationChance->setValue(doubleV);
                doubleV = e.attribute("gamma").toDouble();
                ui->doubleSpinBoxGamma->setValue(doubleV);
                doubleV = e.attribute("gaussianMutationChance").toDouble();
                ui->doubleSpinBoxGaussianMutationChance->setValue(doubleV);
                doubleV = e.attribute("improvementThreshold").toDouble();
                ui->doubleSpinBoxImprovementThreshold->setValue(doubleV);
                doubleV = e.attribute("watchDogTimerLimit").toDouble();
                ui->doubleSpinBoxWatchDogTimerLimit->setValue(doubleV);

                int intV;
                intV = e.attribute("genomeLength").toInt();
                ui->spinBoxGenomeLength->setValue(intV);
                intV = e.attribute("improvementReproductions").toInt();
                ui->spinBoxImprovementReproductions->setValue(intV);
                intV = e.attribute("maxReproductions").toInt();
                ui->spinBoxMaxReproductions->setValue(intV);
                intV = e.attribute("outputPopulationSize").toInt();
                ui->spinBoxOutputPopulationSize->setValue(intV);
                intV = e.attribute("outputStatsEvery").toInt();
                ui->spinBoxOutputStatsEvery->setValue(intV);
                intV = e.attribute("parentsToKeep").toInt();
                ui->spinBoxParentsToKeep->setValue(intV);
                intV = e.attribute("populationSize").toInt();
                ui->spinBoxPopulationSize->setValue(intV);
                intV = e.attribute("saveBestEvery").toInt();
                ui->spinBoxSaveBestEvery->setValue(intV);
                intV = e.attribute("savePopEvery").toInt();
                ui->spinBoxSavePopEvery->setValue(intV);

            }
        }
        n = n.nextSibling();
    }

}

void GAParametersDialog::activateButtons()
{
    if (ui->comboBoxCrossoverType->currentIndex() == -1 || ui->comboBoxParentSelection->currentIndex() == -1 || ui->comboBoxResizeControl->currentIndex() == -1)
    {
        ui->pushButtonOK->setEnabled(false);
    }
    else
    {
        ui->pushButtonOK->setEnabled(true);
    }
}
