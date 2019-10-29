#include "scraper.h"
#include "downloader.h"

Scraper::Scraper()
{

}

void Scraper::updateIndex(QString id,bool allQualities){
    m_id=id;
    m_allQualities=allQualities;

    FileDownloader* api = new FileDownloader(API_BASE + "/page/format/" + id);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( api, &FileDownloader::downloaded, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(10000);
    loop.exec();

    if(!timer.isActive()){
        qStdErr() << "Timeout while attempting to fetch JSON data (/page/format)" << endl;
        exit(1);
    }
    QString json = QString::fromStdString(api->downloadedData().toStdString());
    media = decodeFormatResponse(json);
    qStdOut() << "____________Detected Movie/Show____________" << endl;
    qStdOut() << "Title:\t" << media.title << endl;
    qStdOut() << "ID:\t" << media.id << endl;
    qStdOut() << "Movie?:\t" << media.isMovie << endl;
    qStdOut() << "___________________________________________" << endl;
}

void Scraper::begin(){
    if(media.isMovie){
        FileDownloader* apim = new FileDownloader(API_BASE + media.movie_url);
        QTimer timerm;
        timerm.setSingleShot(true);
        QEventLoop loopm;
        connect( apim, &FileDownloader::downloaded, &loopm, &QEventLoop::quit );
        connect( &timerm, &QTimer::timeout, &loopm, &QEventLoop::quit );
        timerm.start(10000);
        loopm.exec();

        if(!timerm.isActive()){
            qStdErr() << "Timeout while attempting to fetch JSON data (movie)" << endl;
            exit(1);
        }

        QString jsonm = QString::fromStdString(apim->downloadedData().toStdString());
        movieObject_t movie = decodeHighlightModule(jsonm);
        movie.title = media.title;
        QString m3u_streams = getM3UContainer(movie);
        QVector<qualityObject_t>qualities = getQualities(m3u_streams);
        if(m_allQualities){
            for (qualityObject_t q:qualities) {
                qStdOut() << "Downloading " << q.resolution << endl;
                QString playlist = getM3UPlaylist(q,movie);
                QFile file(movie.title + " - " + q.resolution + ".m3u8");
                if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream stream(&file);
                    stream << playlist;
                    file.close();
                }
            }
        }
        else{
            qualityObject_t best = findBestQuality(qualities);
            qStdOut() << "Downloading " << best.resolution << endl;
            QString playlist = getM3UPlaylist(best,movie);
            QFile file(movie.title + " - " + best.resolution + ".m3u8");
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream stream(&file);
                stream << playlist;
                file.close();
            }
        }
    }
    else{
        qStdErr() << "Only movies supported at the moment" << endl;
        exit(1);
    }
    emit finished();
}

mediaObject_t Scraper::decodeFormatResponse(QString json){
    mediaObject_t media;

    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = d.object();

    QJsonValue code = obj.value(QString("code"));
    if(code.toString()=="404"){
        qStdErr() << "Series/Media ID not found" << endl;
        exit(1);
    }

    QJsonValue id = obj.value(QString("id"));
    media.id = id.toInt();
    QJsonValue title = obj.value(QString("title"));
    media.title = title.toString();

    QJsonArray modules = obj.value(QString("modules")).toArray();
    int foundNavigationModule = -1;
     int foundHighlightModule = -1;
    for(int i = 0; i < modules.count() ; i++) {
        QJsonObject module = modules.at(i).toObject();
        QJsonValue type = module.value("type");

        if(type.toString()=="episode_navigation"){
            foundNavigationModule = i;

            QJsonObject navigation = module.value("navigation").toObject();
            if(navigation.value("navigationType").toString()!="season")
                qStdErr() << "Unknown navigation type, continuing anyway..." << endl;

            QJsonArray seasons = obj.value(QString("seasonItems")).toArray();
            QVector<seasonObject_t> season_data;
            for (int s = 0; s < seasons.count(); s++) {
                QJsonObject season = seasons.at(0).toObject();
                seasonObject_t seasonObj;
                seasonObj.id = season.value("id").toInt();
                seasonObj.url = season.value("url").toString();
                season_data.push_back(seasonObj);
            }
            media.seasons = season_data;
        }
        else if(type.toString()=="format_highlight"){
            foundHighlightModule = i;

            QJsonValue mov_url = module.value("moduleUrl");
            media.movie_url = mov_url.toString();
        }
    }

    if(foundNavigationModule >= 0){
        media.isMovie = false;
    }
    else if(foundHighlightModule >= 0){
        media.isMovie = true;
    }
    else{
        qStdErr() << "Error: Neither navigation nor highlight module found. Unable to continue" << endl;
        exit(1);
    }

    return media;
}
movieObject_t Scraper::decodeHighlightModule(QString json){
    movieObject_t movie;

    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = d.object().value("teaser").toObject();

    QJsonValue id = obj.value(QString("id"));
    movie.id = id.toInt();
    QJsonValue title = obj.value(QString("headline"));
    movie.title = title.toString();

    QJsonObject manifest = obj.value(QString("video")).toObject().value("manifest").toObject();
    QJsonValue hlsfairplayhd = manifest.value("hlsfairplayhd");
    QJsonValue hlsfairplay = manifest.value("hlsfairplay");
    if(hlsfairplayhd.toString()!="")
        movie.hlscontainer = hlsfairplayhd.toString();
    else if(hlsfairplay.toString()!="")
        movie.hlscontainer = hlsfairplay.toString();
    else {
        qStdErr() << "Unable to extract HLS url from response (highlight module)" << endl;
        exit(1);
    }

    return movie;
}
QString Scraper::getM3UContainer(movieObject_t movie){
    QString url(movie.hlscontainer);
    url = url.split("?").first(); //remove url parameters

    FileDownloader* api = new FileDownloader(url);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( api, &FileDownloader::downloaded, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(10000);
    loop.exec();

    if(!timer.isActive()){
        qStdErr() << "Timeout while attempting to fetch M3U HLS index container" << endl;
        exit(1);
    }

    url = url.section('/', 0,url.count(QChar('/'))-1); //remove m3u8 filename

    QString m3u8 = QString::fromStdString(api->downloadedData().toStdString());
    m3u8.replace("keyframes/"+QString::number(movie.id),"%KEYFRAME%");
    m3u8.replace(QString::number(movie.id),url + "/" + QString::number(movie.id));
    m3u8.replace("%KEYFRAME%",url + "/keyframes/" + QString::number(movie.id));

    return m3u8;
}
QVector<qualityObject_t> Scraper::getQualities(QString m3u){
    QString line;
    QTextStream stream(&m3u);
    QString incomingUrl = "";
    QVector<qualityObject_t> qualities;
    while (stream.readLineInto(&line)) {
        if(line.startsWith("#EXT-X-STREAM-INF:")){
            int index = line.indexOf("RESOLUTION");
            if(index==-1)continue;
            line.remove(0,index);
            line = line.split("=").at(1);
            QString resolution = line.split(",").first();
            incomingUrl = resolution;
        }
        else if(line.startsWith("http")&&incomingUrl!=""){
            qualityObject_t q;
            q.resolution = incomingUrl;
            q.url = line;
            qualities.push_back(q);
            incomingUrl="";
        }

    }
    stream.seek(0);
    return qualities;
}
qualityObject_t Scraper::findBestQuality(QVector<qualityObject_t> vector){
    qualityObject_t best;
    best.resolution = "0x0";
    for (qualityObject_t q:vector) {
        QString height = q.resolution.split("x").first();
        QString currentbest = best.resolution.split("x").first();
        if(height.toInt() > currentbest.toInt())
            best = q;
    }
    return best;
}

QString Scraper::getM3UPlaylist(qualityObject_t q,movieObject_t movie){
    QString url(q.url);
    url = url.split("?").first(); //remove url parameters

    FileDownloader* api = new FileDownloader(url);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( api, &FileDownloader::downloaded, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(10000);
    loop.exec();

    if(!timer.isActive()){
        qStdErr() << "Timeout while attempting to fetch M3U HLS playlist container" << endl;
        exit(1);
    }

    url = url.section('/', 0,url.count(QChar('/'))-1); //remove m3u8 filename

    QString m3u8 = QString::fromStdString(api->downloadedData().toStdString());
    m3u8.replace(QString::number(movie.id),url + "/" + QString::number(movie.id));

    return m3u8;
}

//Overview -> https://app.tvnow.de/page/format/12483

//Season -> https://app.tvnow.de/module/teaserrow/format/episode/17333?season=1

//Movie -> https://app.tvnow.de/module/teaserrow/format/highlight/12483
