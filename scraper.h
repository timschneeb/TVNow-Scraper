#ifndef SCRAPER_H
#define SCRAPER_H
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QEventLoop>
#include <QVector>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

inline QTextStream& qStdOut()
{
    static QTextStream ts( stdout );
    return ts;
}
inline QTextStream& qStdErr()
{
    static QTextStream ts( stderr );
    return ts;
}

#define API_BASE QString("https://app.tvnow.de")

typedef struct seasonObject_s{
    int id;
    QString url;
}seasonObject_t;
typedef struct mediaObject_s{
    QString title;
    int id;
    bool isMovie;
    //--Movie
    QString movie_url;
    //--Series
    QVector<seasonObject_t> seasons;
}mediaObject_t;
typedef struct movieObject_s{
    QString title;
    int id;
    QString hlscontainer;
}movieObject_t;

typedef struct episodeObject_s{
    QString title;
    int id;
    int season;
    int episode;
    QString hlscontainer;
}episodeObject_t;
typedef struct seriesObject_s{
    QString title;
    int id;
    QVector<episodeObject_t> episodes;
}seriesObject_t;

typedef struct qualityObject_s{
    QString resolution;
    QString url;
}qualityObject_t;

class Scraper : public QObject
{
    Q_OBJECT
public:
    Scraper();
    void updateIndex(QString id,bool allQualities);
    void begin();

signals:
    void finished();

private:
    mediaObject_t media;
    QString m_id;
    bool m_allQualities = false;

    mediaObject_t decodeFormatResponse(QString json);
    movieObject_t decodeHighlightModule(QString json);
    QString getM3UContainer(movieObject_t movie);
    QString getM3UPlaylist(qualityObject_t q,movieObject_t movie);
    QVector<qualityObject_t> getQualities(QString m3u_container);
    qualityObject_t findBestQuality(QVector<qualityObject_t> vector);
};

#endif // SCRAPER_H
