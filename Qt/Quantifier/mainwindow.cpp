#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

int getArea(int axis, int islice, float *area);
int getVolume(float *volume);
int histology(int axis, int islice, int *np, float *area);
int setup(char *input_amfile, char *close_file, char *result_file, float vsize[]);
int getCloseSize(int nvoxels[]);
bool isSetup();
void reset();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	ui->textEdit->setReadOnly(true);
    QString infoFile = QCoreApplication::applicationDirPath() + "/info/quantifier_info.txt";
	QFile file(infoFile);
	bool ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
	if (!ok) {
		ui->textEdit->append("The information file is missing:");
		ui->textEdit->append(infoFile);
    } else {
        QTextStream in(&file);
        QString line = in.readLine();
        while (!line.isNull()) {
            ui->textEdit->append(line);
            line = in.readLine();
        }
        file.close();
        ui->textEdit->moveCursor(QTextCursor::Start);
    }
    is_ready = false;
    is_amfile = false;
    is_closefile = false;
    is_resultfile = false;
    ui->pushButton_area->setEnabled(false);
    ui->pushButton_vessels->setEnabled(false);
    checkReady();
    reset();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::amFileSelecter()
{
	ui->labelResult->setText("");
    amFileName = QFileDialog::getOpenFileName(this,
        tr("Input Amira file"), ".", tr("Amira Files (*.am)"));
    ui->labelAmFile->setText(amFileName);
    is_amfile = true;
    checkReady();
    reset();
}

void MainWindow::closeFileSelecter()
{
	ui->labelResult->setText("");
    closeFileName = QFileDialog::getOpenFileName(this,
        tr("Input close image binary file"), ".", tr("Bin Files (*.bin)"));
    ui->labelCloseFile->setText(closeFileName);
    is_closefile = true;
    checkReady();
    reset();
}

void MainWindow::resultFileSelecter()
{
    ui->labelResult->setText("");
    resultFileName = QFileDialog::getSaveFileName(this,
        tr("Result file"), ".", tr("Result Files (*.out)"));
    ui->labelResultFile->setText(resultFileName);
    is_resultfile = true;
    checkReady();
    reset();
}

void MainWindow::voxelChanged()
{
    reset();
    checkReady();
}

void MainWindow::checkReady()
{
    bool voxelOK;
    voxelsize[0] = ui->lineEdit_xvoxel->text().toFloat();
    voxelsize[1] = ui->lineEdit_yvoxel->text().toFloat();
    voxelsize[2] = ui->lineEdit_zvoxel->text().toFloat();
    if (voxelsize[0] > 0 && voxelsize[1] > 0 && voxelsize[2] > 0)
        voxelOK = true;
    else
        voxelOK = false;
    if (voxelOK && is_amfile && is_closefile && is_resultfile) {
        is_ready = true;
        ui->pushButton_area->setEnabled(true);
        ui->pushButton_vessels->setEnabled(true);
        doSetup();
    } else {
        is_ready = false;
        ui->pushButton_area->setEnabled(false);
        ui->pushButton_vessels->setEnabled(false);
    }
}

void MainWindow::computeVolume()
{
    int err;
    float volume;
    QString volumestr;

    if (!is_ready) {
        return;
    }
    if (!isSetup()) {
        doSetup();
    }
    err = getVolume(&volume);
    volume = 1.0e-9*volume;   // convert um3 -> mm3
    volumestr = QString::number(volume,'f',3);
    ui->lineEdit_volume->setText(volumestr);
}

void MainWindow::computeArea()
{
    int err;
    float area;
    int axis, islice;
    QString areastr;

    if (!is_ready) {
        return;
    }
    if (!isSetup()) {
        doSetup();
    }
    if (ui->radioButton_xaxis->isChecked())
        axis = 0;
    else if (ui->radioButton_yaxis->isChecked())
        axis = 1;
    else if (ui->radioButton_zaxis->isChecked())
        axis = 2;
    islice = ui->lineEdit_intercept->text().toInt();
    if (ui->radioButton_microns->isChecked())
        islice = (int)(islice/voxelsize[axis]);
    err = checkSlice(axis,islice);
    if (err == 0)
        err = getArea(axis,islice,&area);
    else
        area = 0;
    area = 1.0e-6*area;   // convert um2 -> mm2
    areastr = QString::number(area,'f',3);
    ui->lineEdit_area->setText(areastr);
    ui->lineEdit_count->setText("");
    ui->lineEdit_density->setText("");
}

void MainWindow::computeVessels()
{
    int err;
    float area;
    int axis, islice, count, density;
    QString areastr, countstr, densitystr;

    if (!is_ready) {
        return;
    }
    if (!isSetup()) {
        doSetup();
    }
    if (ui->radioButton_xaxis->isChecked())
        axis = 0;
    else if (ui->radioButton_yaxis->isChecked())
        axis = 1;
    else if (ui->radioButton_zaxis->isChecked())
        axis = 2;
    islice = ui->lineEdit_intercept->text().toInt();
    if (ui->radioButton_microns->isChecked())
        islice = (int)(islice/voxelsize[axis]);
    err = checkSlice(axis,islice);
    if (err == 0) {
        err = histology(axis,islice,&count,&area);
    } else {
        area = 0;
        count = 0;
    }
    area = 1.0e-6*area;   // convert um2 -> mm2
    areastr = QString::number(area,'f',3);
    ui->lineEdit_area->setText(areastr);
    countstr = QString::number(count);
    ui->lineEdit_count->setText(countstr);
    if (area > 0)
        density = (int)(count/area + 0.5);
    else
        density = 0;
    densitystr = QString::number(density);
    ui->lineEdit_density->setText(densitystr);
}

int MainWindow::checkSlice(int axis, int islice)
{
    if (islice < 1 || islice > nvoxels[axis]) {
        // need a message to the user
        return 1;
    } else {
        return 0;
    }
}

void MainWindow::doSetup()
{
    int err;
    int um;
    QString numstr, resultstr;
    char input_amfile[1024], close_file[1024], result_file[1024];

    strcpy(input_amfile, amFileName.toAscii().constData());
    strcpy(close_file, closeFileName.toAscii().constData());
    strcpy(result_file, resultFileName.toAscii().constData());
    err = setup(input_amfile,close_file,result_file,voxelsize);
    resultstr = QString::number(err);
    ui->labelResult->setText(resultstr);
    getCloseSize(nvoxels);
    numstr = QString::number(nvoxels[0]);
    ui->lineEdit_nx->setText(numstr);
    numstr = QString::number(nvoxels[1]);
    ui->lineEdit_ny->setText(numstr);
    numstr = QString::number(nvoxels[2]);
    ui->lineEdit_nz->setText(numstr);
    um = (int)(nvoxels[0]*voxelsize[0] + 0.5);
    numstr = QString::number(um);
    ui->lineEdit_umx->setText(numstr);
    um = (int)(nvoxels[1]*voxelsize[1] + 0.5);
    numstr = QString::number(um);
    ui->lineEdit_umy->setText(numstr);
    um = (int)(nvoxels[2]*voxelsize[2] + 0.5);
    numstr = QString::number(um);
    ui->lineEdit_umz->setText(numstr);
    computeVolume();
    //	res = system(cmdstr);
//	if (res == 0)
//		resultstr = "SUCCESS";
//	else if (res == 1)
//		resultstr = "FAILED: wrong number of arguments";
//	else if (res == 2)
//		resultstr = "FAILED: Read error on input file";
//	else if (res == 3)
//		resultstr = "FAILED: Write error on output file";
//	else if (res == 4)
//		resultstr = "FAILED: out of memory";
//	ui->labelResult->setText(resultstr);
}
