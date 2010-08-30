#include "melodiq.h"
//#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QSettings>
#include <QDir>
#ifdef QTOPIA
#include <QSoftMenuBar>
#endif

#define APP_TITLE tr("MelodiQ")
#define SAMPLE1_FILE QString("/tmp/melodiq-sample1.wav")
#define SAMPLE2_FILE QString("/tmp/melodiq-sample2.wav")
#define CAPTCHA_FILE QString("/tmp/melodiq-captcha.png")
#define TAGS_FILE    QString("/tmp/melodiq-tags.txt")
#ifdef QTOPIA
    #define SAMPLE_LENGTH 15
#else
    #define SAMPLE_LENGTH 5
#endif

MelodiqMainWindow::MelodiqMainWindow(QWidget* parent, Qt::WFlags f)
    : QWidget(parent, f)
{
    setWindowTitle(APP_TITLE);

#ifdef QTOPIA
    // Construct context menu, available to the user via Qtopia's soft menu bar.
    QMenu *menu = QSoftMenuBar::menuFor(this);
    Q_UNUSED(menu); // Normally, we would use "menu" to add more actions.
    // The context menu will not be active unless it has at least one action.
    // Tell Qtopia to provide the "Help" option, which will enable the user to
    // read the Help documentation for this application.
    QSoftMenuBar::setHelpEnabled(this,true);
#else
    this->setMinimumSize(360,360);
#endif

    bStart = new QPushButton(this);
    bCancel = new QPushButton(this);
    bQuit = new QPushButton(this);
    bConfirm = new QPushButton(this);
    bTube = new QPushButton(this);
    bRetry = new QPushButton(this);
    bSearchGoogle = new QPushButton(this);
    bSearchYoutube = new QPushButton(this);
    bStart->setText("Start");
    bCancel->setText("Cancel");
    bQuit->setText("Quit");
    bConfirm->setText("Confirm");
    bTube->setText("Watch Most Relevant Youtube Video");
    bRetry->setText("Retry");
    bSearchGoogle->setText("Search Google");
    bSearchYoutube->setText("Search Youtube");
    connect(bStart, SIGNAL(clicked()), this, SLOT(startClicked()));
    connect(bCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(bQuit, SIGNAL(clicked()), this, SLOT(quitClicked()));
    connect(bConfirm, SIGNAL(clicked()), this, SLOT(confirmClicked()));
    connect(bTube, SIGNAL(clicked()), this, SLOT(tubeClicked()));
    connect(bRetry, SIGNAL(clicked()), this, SLOT(retryClicked()));
    connect(bSearchGoogle, SIGNAL(clicked()), this, SLOT(searchGoogleClicked()));
    connect(bSearchYoutube, SIGNAL(clicked()), this, SLOT(searchYoutubeClicked()));

    lStatus = new QLabel(this);
    lComment = new QLabel(this);
    lCaptcha = new QLabel(this);
    lStatus->setAlignment(Qt::AlignHCenter);
    lComment->setAlignment(Qt::AlignHCenter);
    lCaptcha->setAlignment(Qt::AlignCenter);
    lStatus->setWordWrap(true);
    lComment->setWordWrap(true);

    rexp = new QRegExpValidator(QRegExp("^-{0,1}\\d+"), this);

    code = new QLineEdit(this);
    code->setValidator(rexp);
    code->setMaxLength(5);
    connect(code, SIGNAL(returnPressed()), this, SLOT(confirmClicked()));

    info = new QWebView(this);
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(255, 255, 255, 255));
    p.setColor(QPalette::Background, QColor(255, 255, 255, 255));
    info->setPalette(p);
    info->setAttribute(Qt::WA_OpaquePaintEvent, false); //
    info->setTextSizeMultiplier(1.5);

    layout = new QGridLayout(this);
    layout->addWidget(lStatus, 0, 0, 1, 2);
    layout->addWidget(lComment, 1, 0, 1, 2);
    layout->addWidget(lCaptcha, 2, 0, 1, 2);
    layout->addWidget(code, 3, 0, 1, 2);
    layout->addWidget(info, 4, 0, 1, 2);
    layout->addWidget(bStart, 5, 0, 1, 2);
    layout->addWidget(bConfirm, 6, 0, 1, 2);
    layout->addWidget(bSearchGoogle, 7, 0, 1, 1);
    layout->addWidget(bSearchYoutube, 7, 1, 1, 1);
    layout->addWidget(bTube, 8, 0, 1, 2);
    layout->addWidget(bRetry, 9, 0, 1, 2);
    layout->addWidget(bCancel, 10, 0, 1, 2);
    layout->addWidget(bQuit, 11, 0, 1, 2);
    setLayout(layout);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerAction()));

    proc = NULL;
    setStage(StageHome);
}

void MelodiqMainWindow::closeEvent(QCloseEvent *event)
{
    cancelClicked();
    QWidget::closeEvent(event);
}

void MelodiqMainWindow::setStage(Stage s)
{
    stage = s;

    lComment->setVisible(s!=StageResultOk);
    lCaptcha->setVisible(s==StageCaptcha);
    code->setVisible(s==StageCaptcha);
    info->setVisible(s==StageResultOk);
    bStart->setVisible(s==StageHome);
    bConfirm->setVisible(s==StageCaptcha);
    bSearchGoogle->setVisible(s==StageResultOk); //
    bSearchYoutube->setVisible(s==StageResultOk); //
    bTube->setVisible(s==StageResultOk);
    bRetry->setVisible(false);
    bCancel->setVisible(s!=StageHome);
    bQuit->setVisible(s==StageHome);

    lComment->setText("");
    switch (stage)
    {
        case StageHome:
            lComment->setText("Make sure that Internet connection is established. Place your phone near a music source and press \"Start\".");
        case StageRecording:
        case StageEncoding:
        case StageUploading:
            lStatus->setText("Step 1/3: Record an audio sample");
            break;
        case StageGettingCaptcha:
        case StageCaptcha:
            code->setText("");
        case StageSendingCaptcha:
            lStatus->setText("Step 2/3: Prove you're a human");
            break;
        case StageResultOk:
        case StageResultFailed:
            lStatus->setText("Step 3/3: Get a result");
            break;
    }
}

void MelodiqMainWindow::startClicked()
{
#ifdef QTOPIA
    bool depok = false;
    while (true)
    {
        bool curlok = QFile::exists("/usr/bin/curl");
        bool soxok = QFile::exists("/usr/bin/sox");
        QString depstr = QString(QString(curlok ? "" : "curl") + " " + QString(soxok ? "" : "sox")).trimmed();
        depok = curlok && soxok;
        if (depok)
            break;
        else
            if (QMessageBox::question(this, APP_TITLE,
                    tr("The following packages are required: %1. Do you want to install them now?").arg(QString(depstr).replace(" ",", ")),
                    QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            {
                QProcess::execute("raptor -u -i " + depstr);
            }
            else
                return;
    }
#endif

    setStage(StageRecording);
    setMic(true);
#ifdef QTOPIA
    runProc("arecord -D plughw:0,0 -r 11025 " + SAMPLE1_FILE); //8000, 11025, 22050
#else
    runProc("sleep " + QString::number(SAMPLE_LENGTH*10));
#endif
    tick = 0;
    updateRecTime();
    timer->start(1000);
}

void MelodiqMainWindow::setMic(bool enabled)
{
#ifdef QTOPIA
    QProcess::execute( enabled ?
            "alsactl -f /usr/share/openmoko/scenarios/voip-handset.state restore" :
            "alsactl -f /usr/share/openmoko/scenarios/stereoout.state restore");
#else
    Q_UNUSED(enabled);
#endif
}

void MelodiqMainWindow::updateRecTime()
{
    lComment->setText(QString("Recording\n") +
                      QString("00:00:") + QString("%1").arg(tick,2,10,QChar('0')) + QString(" / ") +
                      QString("00:00:") + QString("%1").arg(SAMPLE_LENGTH,2,10,QChar('0')));
}

void MelodiqMainWindow::timerAction()
{
    tick++;
    updateRecTime();
    if (tick>=SAMPLE_LENGTH)
    {
        timer->stop();
        endProc();
        setMic(false);
        setStage(StageEncoding);
        lComment->setText("Encoding...");
        runProc("sox -v 15 " + SAMPLE1_FILE + " " + SAMPLE2_FILE);
    }
}

void MelodiqMainWindow::startUploading()
{
    setStage(StageUploading);
    lComment->setText("Uploading...");
    runProc("curl -F uploadedfile=@" + SAMPLE2_FILE + " -F step=21 http://audiotag.info/index.php");
}

void MelodiqMainWindow::retryClicked()
{
    startUploading();
}

void MelodiqMainWindow::confirmClicked()
{
    QString ct(code->text());
    int p=0;
    if (rexp->validate(ct, p) == QValidator::Acceptable)
    {
        setStage(StageSendingCaptcha);
        lComment->setText("Analyzing...");
        runProc("curl -F PHPSESSID=" + sid + " -F step=3 -F uploadedfilename=" + ufname + " -F capt=" + code->text() + " http://audiotag.info/index.php");
    }
}

QString MelodiqMainWindow::getSearchString()
{
    return QString(artist + " " + title).trimmed();
}

void MelodiqMainWindow::searchGoogleClicked()
{
    QProcess *extproc = new QProcess(this);
    extproc->start("arora http://www.google.com/search?q=" + QUrl::toPercentEncoding(getSearchString()));
}

void MelodiqMainWindow::searchYoutubeClicked()
{
    QProcess *extproc = new QProcess(this);
    extproc->start("arora http://www.youtube.com/results?search_query=" + QUrl::toPercentEncoding(getSearchString()));
}

void MelodiqMainWindow::tubeClicked()
{
    QSettings settings(QDir::homePath() + "/Settings/arora-browser.org/Arora.conf", QSettings::IniFormat);
    
    settings.beginGroup(QLatin1String("video"));
    bool fbdev = settings.value(QLatin1String("fbdev"), true).toBool();
    bool framedrop = settings.value(QLatin1String("framedrop"), true).toBool();
    bool center = settings.value(QLatin1String("center"), true).toBool();
    bool rotate = settings.value(QLatin1String("rotate"), false).toBool();
    QString mpargs1 = composeMplayerArgs(fbdev, framedrop, center, rotate);
    QString mpargs2 = settings.value(QLatin1String("mpargs2")).toString();
    QString ytargs = settings.value(QLatin1String("ytargs")).toString();

    QStringList qsl;
    
    QString mpargs = (mpargs1 + " " + mpargs2.trimmed()).trimmed();
    if (!mpargs.isEmpty())
        qsl << "--mpargs" << mpargs;

    ytargs = ytargs.trimmed();
    if (!ytargs.isEmpty())
        ytargs += " ";
    qsl << "--youtube-dl" << ytargs + "\"ytsearch:" + getSearchString() + "\"";

    QProcess *extproc = new QProcess(this);
    extproc->start("qmplayer", qsl);
}

QString MelodiqMainWindow::composeMplayerArgs(bool fbdev, bool framedrop, bool center, bool rotate)
{
    QString res = QString(fbdev ? "-vo fbdev ":"") +
            QString(framedrop ? "-framedrop ":"") +
            QString(center ? "-geometry 50%:40% ":"") +
            QString(rotate ? "-vf rotate=2 ":"");
    return res.trimmed();
}

void MelodiqMainWindow::cancelClicked()
{
    if (timer->isActive())
        timer->stop();
    endProc(); //
    if (stage == StageRecording)
        setMic(false);
    setStage(StageHome);
}

void MelodiqMainWindow::quitClicked()
{
    close();
}

//-----------------------------------------------------------------------------

void MelodiqMainWindow::runProc(QString cmd)
{
    //qDebug() << cmd;
    outbuf = "";
    proc = new QProcess(this);
    //proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(pReadyReadStandardOutput()));
    //connect(proc, SIGNAL(readyReadStandardError()), this, SLOT(pReadyReadStandardError()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(pFinished(int, QProcess::ExitStatus)));
    proc->start(cmd);
    if(!proc->waitForStarted())
    {
        QMessageBox::critical(this, APP_TITLE, tr("Unable to start sub-process"));
        cancelClicked();
    }
}

void MelodiqMainWindow::endProc()
{
    if (proc!=NULL)
        disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(pFinished(int, QProcess::ExitStatus)));
    delete proc;
    proc = NULL;
}

void MelodiqMainWindow::pReadyReadStandardOutput()
{
    outbuf += QString::fromUtf8(proc->readAllStandardOutput());
}

void MelodiqMainWindow::pFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    //qDebug() << outbuf;
    if (exitStatus == QProcess::NormalExit)
    {
        endProc();
        switch (stage)
        {
            case StageEncoding:
                startUploading();
                break;
            case StageUploading:
                sid = extractVal("name=\"PHPSESSID\" value=\"", "\"");
                ufname = extractVal("name=\"uploadedfilename\" value=\"", "\"");
                //qDebug() << sid << ufname;
                if (sid.isEmpty() || ufname.isEmpty())
                {
                    QMessageBox::critical(this, APP_TITLE, tr("Server has rejected the request"));
                    cancelClicked();
                }
                else
                {
                    setStage(StageGettingCaptcha);
                    lComment->setText("Getting captcha...");
                    QFile::remove(CAPTCHA_FILE);
                    runProc("curl http://audiotag.info/captcha/captcha_img.php?PHPSESSID=" + sid + " -o " + CAPTCHA_FILE);
                }
                break;
            case StageGettingCaptcha:
                {
                    QPixmap pixmap(CAPTCHA_FILE);
                    if (pixmap.isNull())
                    {
                        QMessageBox::critical(this, APP_TITLE, tr("Server has not returned the captcha image"));
                        cancelClicked();
                    }
                    else
                    {
                        setStage(StageCaptcha);
                        lComment->setText("Please calculate the expression and enter the result");
                        lCaptcha->setPixmap(pixmap);
                    }
                }
                break;
            case StageSendingCaptcha:
                if (outbuf.indexOf("<table border=\"0\" class=\"restable\">") >= 0)
                {
                    percent = extractVal("<SPAN class=\"percent\">", "</SPAN>").trimmed();
                    title = extractVal("<strong> Title: </strong></td><td>", "</td>").trimmed();
                    art = extractVal("target=\"_blank\"> <img src=\"", "\"").trimmed();
                    artist = extractVal("<strong> Artist: </strong></td><td>", "</td>").trimmed();
                    album = extractVal("<strong> Album:  </strong></td><td>", "</td>").trimmed();
                    year = extractVal("<strong> Year:   </strong></td><td>", "</td>").trimmed();

                    setStage(StageResultOk);
                    QString res =
                        "<HTML>" "\n"
                        "<BODY>" "\n"
                        "<table border=\"0\" align=\"center\">" "\n"
                        "    <th colspan=\"3\"><SPAN>%1</SPAN></th>" "\n"
                        "    <tr>" "\n"
                        "        <td align=\"right\"><strong> Title: </strong></td><td>%2</td>" "\n"
                        "        <td rowspan=\"5\" align=\"right\"> %3 </td>" "\n"
                        "    </tr>" "\n"
                        "    <tr><td align=\"right\"><strong> Artist: </strong></td><td>%4</td></tr>" "\n"
                        "    <tr><td align=\"right\"><strong> Album:  </strong></td><td>%5</td></tr>" "\n"
                        "    <tr><td align=\"right\"><strong> Year:   </strong></td><td>%6</td></tr>" "\n"
                        "</table>" "\n"
                        "</BODY>" "\n"
                        "</HTML>" "\n";
                    res = res.arg(percent, title,
                                  art.isEmpty() ? "" : "<img src=\"" + art + "\" />",
                                  artist, album, year);
                    info->setHtml(res);
                    writeTag();
                }
                else
                {
                    QString msg = "";
                    setStage(StageResultFailed);
                    if (outbuf.indexOf("Sorry, our database doesn't have anything matching your file. Cannot identify.") >= 0)
                    {
                        msg = "The database doesn't have anything matching your music sample. Cannot identify.";
                    }
                    else if (outbuf.indexOf("Sorry, your answer is incorrect. Are you a robot?") >= 0)
                    {
                        msg = "Error: Your answer is incorrect.";
                        bRetry->setVisible(true);
                    }
                    else if (outbuf.indexOf("Sorry, cannot proceed with file analysis: zero audio signal or file is too short.") >= 0)
                    {
                        msg = "Error: Cannot proceed with analysis: zero audio signal or sample is too short.";
                    }
                    else if (outbuf.indexOf("Sorry, cannot process your file: unsupported file format or broken file.") >= 0)
                    {
                        msg = "Error: Cannot process your music sample: unsupported file format or broken file.";
                    }
                    else
                    {
                        msg = "Unknown error";
                    }
                    lComment->setText(msg);
                }
                break;
            default:
                break;
        }
    }
    else
    {
        QMessageBox::critical(this, APP_TITLE, tr("Sub-process returned an error code"));
        cancelClicked();
    }
}

QString MelodiqMainWindow::extractVal(QString str1, QString str2)
{
    int p1 = outbuf.indexOf(str1);
    if (p1>=0)
    {
        p1 += str1.length();
        int p2 = outbuf.indexOf(str2,p1);
        if (p2>=0)
        {
            int n = p2-p1;
            return outbuf.mid(p1,n);
        }
    }
    return "";
}

void MelodiqMainWindow::writeTag()
{
    QString curtime = QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss");
    QString tag =
        "%1" "\n"
        "Title:  %2" "\n"
        "Artist: %3" "\n"
        "Album:  %4" "\n"
        "Year:   %5" "\n"
        "\n";
    tag = tag.arg(curtime, title, artist, album, year);

    QFile f(TAGS_FILE);
    if (f.open(QIODevice::Append))
    {
        QTextStream outs(&f);
        outs << tag;
        f.close();
    }
}
