#ifndef PARSER
#define PARSER

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QEventLoop>

#include "json.hpp"


class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser(QObject *parent = nullptr);

    void request(const QString &barcode);
    int web_request_sync(const QString &barcode);

    QString getProductName() const { return m_name; }
    QString getBrand() const { return m_brand; }
    QString getQuantity() const { return m_quantity; }
    QStringList getAllergens() const { return m_allergens; }

signals:
    void productReady(const QString &name, const QString &brand);
    void allergensReady(const QStringList &allergens);
    void errorOccurred(const QString &errorString);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager m_manager;
    QString m_currentBarcode;
    QEventLoop m_loop;
    int m_syncResult = -1;

    QString m_name, m_brand, m_quantity;
    QStringList m_allergens;
};

#endif
