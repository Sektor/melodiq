#ifndef MELODIQ_H
#define MELODIQ_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QWebView>
#include <QLineEdit>
#include <QTimer>
#include <QProcess>
#include <QValidator>

class MelodiqMainWindow : public QWidget
{
    Q_OBJECT
public:
    MelodiqMainWindow(QWidget* parent = 0, Qt::WFlags f = 0);

private:
   enum Stage
    {
        StageHome,
        StageRecording,
        StageEncoding,
        StageUploading,
        StageGettingCaptcha,
        StageCaptcha,
        StageSendingCaptcha,
        StageResultOk,
        StageResultFailed
    };

    QLabel *lStatus;
    QLabel *lComment;
    QLabel *lCaptcha;
    QLineEdit *code;
    QWebView *info;
    QGridLayout *layout;
    QPushButton *bStart;
    QPushButton *bCancel;
    QPushButton *bQuit;
    QPushButton *bConfirm;
    QPushButton *bTube;
    QPushButton *bRetry;
    QPushButton *bSearchGoogle;
    QPushButton *bSearchYoutube;
    QTimer *timer;
    QProcess *proc;
    QRegExpValidator *rexp;

    Stage stage;
    int tick;
    QString outbuf;
    QString sid;
    QString ufname;

    QString percent;
    QString title;
    QString artist;
    QString album;
    QString year;
    QString art;

    void setStage(Stage s);
    void setMic(bool enabled);
    void updateRecTime();
    void startUploading();
    QString extractVal(QString s1, QString s2);

    void runProc(QString cmd);
    void endProc();

private slots:
    void startClicked();
    void cancelClicked();
    void quitClicked();
    void confirmClicked();
    void tubeClicked();
    void timerAction();
    void retryClicked();
    void searchGoogleClicked();
    void searchYoutubeClicked();

    void pReadyReadStandardOutput();
    //void pReadyReadStandardError();
    void pFinished(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    void closeEvent(QCloseEvent *event);
};

#endif //MELODIQ_H
