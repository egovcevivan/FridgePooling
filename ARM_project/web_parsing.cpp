#include "web_parsing.h"
#include "json.hpp"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

using json = nlohmann::json;

Parser::Parser(QObject *parent) : QObject(parent) {}

void Parser::request(const QString &barcode) {
    const QString API_URL_TEMPLATE = "https://world.openfoodfacts.org/api/v2/product/%1.json?fields=product_name,brands,allergens,allergens_tags,traces_tags";
    QString urlString = API_URL_TEMPLATE.arg(barcode);
    QUrl url(urlString);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "FridgePooling/0.4 (httpы://github.com/yourname/FridgePooling)");
    QNetworkReply *reply = m_manager.get(request);
    connect(reply, &QNetworkReply::finished, this, &Parser::onReplyFinished);
}

void Parser::onReplyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        m_loop.quit();
        return;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();

    qDebug() << "API response size:" << data.size();

    try {
        json root = json::parse(data.toStdString());

        if (root.contains("product") && !root["product"].is_null()) {
            const auto& product = root["product"];

            m_name = QString::fromStdString(product.value("product_name", ""));
            m_brand = QString::fromStdString(product.value("brands", ""));

            m_allergens.clear();

            if (product.contains("allergens") && product["allergens"].is_string()) {

                QString allergensStr = QString::fromStdString(product["allergens"].get<std::string>());
                if (!allergensStr.isEmpty()) {
                    QStringList parts = allergensStr.split(',', QString::SkipEmptyParts);
                    for (QString &part : parts) {
                        m_allergens << part.trimmed();
                    }
                }
            }
            if (m_allergens.isEmpty() && product.contains("allergens_tags") && product["allergens_tags"].is_array()) {
                for (const auto& tag : product["allergens_tags"])
                    if (tag.is_string())
                        m_allergens << QString::fromStdString(tag.get<std::string>());
            }

            emit productReady(m_name, m_brand);
            emit allergensReady(m_allergens);
        }
        else {
            emit errorOccurred("No product data found in API response.");
        }
    } catch (const json::parse_error& e) {
        emit errorOccurred(QString("JSON parse error: %1").arg(e.what()));
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Exception: %1").arg(e.what()));
    }

    m_loop.quit();
}

int Parser::web_request_sync(const QString &barcode) {
    m_syncResult = -1;
    disconnect(this, &Parser::productReady, this, nullptr);
    disconnect(this, &Parser::errorOccurred, this, nullptr);

    connect(this, &Parser::productReady, this, [this](const QString &name, const QString &brand){
        m_name = name;
        m_brand = brand;
        m_syncResult = 0;
        m_loop.quit();
    });
    connect(this, &Parser::errorOccurred, this, [this](const QString &){
        m_syncResult = -1;
        m_loop.quit();
    });

    request(barcode);
    m_loop.exec();

    return m_syncResult;
}
