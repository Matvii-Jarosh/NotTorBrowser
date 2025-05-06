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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleServer();
    void readTorOutput();
    void handleTorFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void closeEvent(QCloseEvent *event) override;
    void setupUI();
    void startTor();
    void stopTor();
    void updateUI();
    bool validateInputs();
    void checkForHostnameFile();

    QProcess *torProcess;
    QTextEdit *logView;
    QPushButton *toggleButton;
    QLineEdit *socksPortInput;
    QLineEdit *hiddenServiceDirInput;
    QLineEdit *hiddenServicePortInput;
    QLabel *statusLabel;
    QLineEdit* hostnameDisplay;
    bool isTorRunning = false;
};

#endif // MAINWINDOW_H
