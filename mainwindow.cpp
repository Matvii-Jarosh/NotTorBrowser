/* Copyright (C) 2025 Matvii Jarosh

   This file is part of NotTorBrowser

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */
#include "mainwindow.h"
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QIntValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), torProcess(nullptr)
{
    setWindowTitle("NotTorBrowser - Tor Controller");
    setMinimumSize(700, 500);
    setupUI();
}

MainWindow::~MainWindow()
{
    if (isTorRunning) {
        stopTor();
    }
    delete torProcess;
}

void MainWindow::closeEvent(QCloseEvent *) {
    stopTor();
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    statusLabel = new QLabel("Tor Status: Stopped", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: red;");

    QGroupBox *socksGroup = new QGroupBox("Socks Proxy", this);
    QFormLayout *socksLayout = new QFormLayout(socksGroup);

    socksPortInput = new QLineEdit("9050", this);
    socksPortInput->setValidator(new QIntValidator(1024, 65535, this));
    socksLayout->addRow("Socks Port:", socksPortInput);

    QGroupBox *hiddenServiceGroup = new QGroupBox("Hidden Service", this);
    QFormLayout *hsLayout = new QFormLayout(hiddenServiceGroup);

    hiddenServiceDirInput = new QLineEdit(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/hidden_service",
        this);
    hiddenServicePortInput = new QLineEdit("80 127.0.0.1:5000", this);

    hsLayout->addRow("Service Directory:", hiddenServiceDirInput);
    hsLayout->addRow("Port Mapping:", hiddenServicePortInput);

    toggleButton = new QPushButton("Start Tor", this);
    toggleButton->setStyleSheet("QPushButton { padding: 10px; font-weight: bold; font-size: 14px; }");
    connect(toggleButton, &QPushButton::clicked, this, &MainWindow::toggleServer);

    logView = new QTextEdit(this);
    logView->setReadOnly(true);
    logView->setPlaceholderText("Tor logs will appear here...");

    hostnameDisplay = new QLineEdit(this);
    hostnameDisplay->setReadOnly(true);
    hostnameDisplay->setPlaceholderText(".onion address will appear here after starting");

    QPushButton* copyButton = new QPushButton("Copy", this);
    connect(copyButton, &QPushButton::clicked, [this]() {
        hostnameDisplay->selectAll();
        hostnameDisplay->copy();
        logView->append("[INFO] Address copied to clipboard");
    });

    QHBoxLayout* hostnameLayout = new QHBoxLayout();
    hostnameLayout->addWidget(new QLabel("Your .onion address:", this));
    hostnameLayout->addWidget(hostnameDisplay);
    hostnameLayout->addWidget(copyButton);

    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(socksGroup);
    mainLayout->addWidget(hiddenServiceGroup);
    mainLayout->addWidget(toggleButton);
    mainLayout->addWidget(new QLabel("Tor Log Output:", this));
    mainLayout->addWidget(logView);
    mainLayout->addLayout(hostnameLayout);

    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    torProcess = new QProcess(this);
    connect(torProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readTorOutput);
    connect(torProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleTorFinished);
}

void MainWindow::toggleServer()
{
    if (isTorRunning) {
        stopTor();
    } else {
        if (validateInputs()) {
            startTor();
        }
    }
}

bool MainWindow::validateInputs()
{
    if (socksPortInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter Socks port number");
        return false;
    }

    if (hiddenServiceDirInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter Hidden Service directory");
        return false;
    }

    if (hiddenServicePortInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter Hidden Service port mapping");
        return false;
    }

    return true;
}

void MainWindow::startTor()
{
    QString torPath = QDir::currentPath() + "/tor/tor.exe";
    QString configPath = QDir::currentPath() + "/torrc.txt";

    if (!QFileInfo::exists(torPath)) {
        logView->append("[ERROR] Tor executable not found at: " + torPath);
        QMessageBox::critical(this, "Error", "Tor executable not found!");
        return;
    }

    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logView->append("[ERROR] Failed to create config file");
        QMessageBox::critical(this, "Error", "Cannot create configuration file!");
        return;
    }

    QTextStream out(&configFile);
    out << "SocksPort " << socksPortInput->text() << "\n";
    out << "Log notice stdout\n";
    out << "AvoidDiskWrites 1\n";
    out << "HiddenServiceDir " << hiddenServiceDirInput->text() << "\n";
    out << "HiddenServicePort " << hiddenServicePortInput->text() << "\n";
    configFile.close();

    logView->append(QString("[%1] Starting Tor...").arg(QDateTime::currentDateTime().toString()));
    logView->append("Config file content:\n" + configFile.readAll());

    torProcess->start(torPath, QStringList() << "-f" << configPath);

    if (!torProcess->waitForStarted()) {
        logView->append("[ERROR] Failed to start Tor process");
        QMessageBox::critical(this, "Error", "Failed to start Tor process!");
        return;
    }

    isTorRunning = true;
    updateUI();
}

void MainWindow::stopTor()
{
    if (torProcess && isTorRunning) {
        logView->append(QString("[%1] Stopping Tor...").arg(QDateTime::currentDateTime().toString()));
        torProcess->terminate();

        if (!torProcess->waitForFinished(3000)) {
            torProcess->kill();
            logView->append("[WARNING] Tor process was forcibly killed");
        }

        isTorRunning = false;
        updateUI();
        logView->append(QString("[%1] Tor stopped").arg(QDateTime::currentDateTime().toString()));

        // Clean up config file
        QFile::remove(QDir::currentPath() + "/torrc.txt");
        hostnameDisplay->clear();
    }
}

void MainWindow::readTorOutput()
{
    QByteArray output = torProcess->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(output);
    logView->append(text.trimmed());

    if (text.contains("Done")) {
        QString hostnameFile = hiddenServiceDirInput->text() + "/hostname";
        QFile file(hostnameFile);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString onionAddress = in.readLine().trimmed();
            if (!onionAddress.isEmpty()) {
                hostnameDisplay->setText(onionAddress);
                logView->append("[INFO] Retrieved .onion address from file: " + onionAddress);
            }
            file.close();
        }
    }
}

void MainWindow::handleTorFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        logView->append("[ERROR] Tor process crashed!");
    } else {
        logView->append(QString("[INFO] Tor process exited with code %1").arg(exitCode));
    }

    isTorRunning = false;
    updateUI();
}

void MainWindow::updateUI()
{
    if (isTorRunning) {
        statusLabel->setText("Tor Status: RUNNING");
        statusLabel->setStyleSheet("color: green; font-weight: bold; font-size: 16px;");
        toggleButton->setText("Stop Tor");
        socksPortInput->setEnabled(false);
        hiddenServiceDirInput->setEnabled(false);
        hiddenServicePortInput->setEnabled(false);
    } else {
        statusLabel->setText("Tor Status: STOPPED");
        statusLabel->setStyleSheet("color: red; font-weight: bold; font-size: 16px;");
        toggleButton->setText("Start Tor");
        socksPortInput->setEnabled(true);
        hiddenServiceDirInput->setEnabled(true);
        hiddenServicePortInput->setEnabled(true);
    }
}
