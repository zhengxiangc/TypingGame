/* -------------------------------------------------------------------------
//  文件名      :  deepseekwordclient.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  QNetworkAccessManager-based Deepseek chat client with local word fallback.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "deepseekwordclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrl>

namespace {
constexpr char kDeepSeekUrl[] = "https://api.deepseek.com/v1/chat/completions";

QString resolveApiKey()
{
    const QByteArray fromEnv = qgetenv("Deepseek_Api");
    if (fromEnv.isEmpty()) {
        return {};
    }
    return QString::fromUtf8(fromEnv).trimmed();
}
QString truncateForLog(const QString& s, int maxChars)
{
    if (s.size() <= maxChars) {
        return s;
    }
    return s.left(maxChars) + QStringLiteral("...");
}
} // namespace (file-local helpers)

KCTDeepSeekWordClient::KCTDeepSeekWordClient(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString KCTDeepSeekWordClient::pickLocalWord(int minLength, int maxLength)
{
    static const QStringList words{
        QStringLiteral("NEBULA"),
        QStringLiteral("QUASAR"),
        QStringLiteral("ORBIT"),
        QStringLiteral("COMET"),
        QStringLiteral("COSMOS"),
        QStringLiteral("GALAXY"),
        QStringLiteral("METEOR"),
        QStringLiteral("ROCKET"),
        QStringLiteral("STELLAR"),
        QStringLiteral("PLANET"),
        QStringLiteral("AURORA"),
        QStringLiteral("CORAL"),
        QStringLiteral("DOLPHIN"),
        QStringLiteral("MARINE"),
        QStringLiteral("TUNA"),
        QStringLiteral("WHALE"),
        QStringLiteral("SUMMIT"),
        QStringLiteral("GLACIER"),
        QStringLiteral("CANYON"),
        QStringLiteral("FOREST"),
        QStringLiteral("MEADOW"),
        QStringLiteral("RHYTHM"),
        QStringLiteral("SONATA"),
        QStringLiteral("GUITAR"),
        QStringLiteral("MELODY"),
        QStringLiteral("SENSOR"),
        QStringLiteral("VECTOR"),
        QStringLiteral("BINARY"),
        QStringLiteral("MATRIX"),
        QStringLiteral("STADIUM"),
        QStringLiteral("RUNNER"),
        QStringLiteral("TENNIS"),
        QStringLiteral("HOCKEY"),
    };
    const int lo = qMax(3, minLength);
    const int hi = qMax(lo, maxLength);
    for (int nAttempt = 0; nAttempt < 40; ++nAttempt) {
        const QString w = words.at(QRandomGenerator::global()->bounded(0, words.size()));
        if (w.size() >= lo && w.size() <= hi) {
            return w;
        }
    }
    return QStringLiteral("ORBIT");
}

QStringList KCTDeepSeekWordClient::extractWordsFromModelContent(const QString& content, int minLength, int maxLength,
                                                                int maxCount)
{
    QStringList out;
    if (maxCount <= 0) {
        return out;
    }
    const int lo = qMax(3, minLength);
    const int hi = qMax(lo, maxLength);

    QString normalized = content;
    normalized.replace(QLatin1Char(','), QLatin1Char('\n'));
    static const QRegularExpression kSep(QStringLiteral("[\\s\\n\\r]+"));
    const QStringList parts = normalized.split(kSep, Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        if (out.size() >= maxCount) {
            break;
        }
        const QString w = sanitizeToSingleWord(part.trimmed(), lo, hi);
        if (!w.isEmpty() && !out.contains(w)) {
            out.append(w);
        }
    }
    return out;
}

QString KCTDeepSeekWordClient::sanitizeToSingleWord(const QString& raw, int minLength, int maxLength)
{
    QString letters;
    letters.reserve(raw.size());
    for (QChar ch : raw) {
        if (ch.isLetter()) {
            letters.append(ch.toUpper());
        } else if (!letters.isEmpty()) {
            break;
        }
    }
    const int lo = qMax(3, minLength);
    const int hi = qMax(lo, maxLength);
    if (letters.size() > hi) {
        letters = letters.left(hi);
    }
    if (letters.size() < lo) {
        return {};
    }
    return letters;
}

void KCTDeepSeekWordClient::emitLocalFallbackBatch(int minLength, int maxLength, int batchSize, const QString& reason,
                                                   const QString& detail)
{
    const int lo = qMax(3, minLength);
    const int hi = qMax(lo, maxLength);
    const int n = qMax(1, batchSize);
    QStringList list;
    list.reserve(n);
    for (int i = 0; i < n; ++i) {
        QString w;
        for (int attempt = 0; attempt < 30; ++attempt) {
            w = pickLocalWord(lo, hi);
            if (!list.contains(w)) {
                break;
            }
        }
        list.append(w);
    }
    const QString sample = list.size() <= 8 ? list.join(QLatin1Char(',')) : list.mid(0, 8).join(QLatin1Char(',')) + QStringLiteral(",...");
    QString msg = QStringLiteral("reward_llm source=local reason=%1 batch=%2 words_sample=%3")
                      .arg(reason)
                      .arg(n)
                      .arg(sample);
    if (!detail.isEmpty()) {
        msg += QStringLiteral(" detail=%1").arg(truncateForLog(detail, 160));
    }
    emit rewardDiagnosticLine(msg);
    emit wordsBatchReady(list);
}

void KCTDeepSeekWordClient::requestWordsBatchAsync(int minLength, int maxLength, int batchSize,
                                                   const QString& domainTopicEnglish)
{
    const QString apiKey = resolveApiKey();
    const int lo = qMax(3, minLength);
    const int hi = qMax(lo, maxLength);
    const int n = qMax(1, batchSize);

    if (apiKey.isEmpty()) {
        emitLocalFallbackBatch(lo, hi, n, QStringLiteral("no_api_key"));
        return;
    }

    emit rewardDiagnosticLine(QStringLiteral("reward_llm http_request batch=%1 len=%2-%3 topic=%4")
                                  .arg(n)
                                  .arg(lo)
                                  .arg(hi)
                                  .arg(truncateForLog(domainTopicEnglish, 120)));

    QJsonObject userMessage;
    userMessage[QStringLiteral("role")] = QStringLiteral("user");
    userMessage[QStringLiteral("content")] = QStringLiteral(
                                                 "Reply with exactly %1 distinct English words, one per line. Each "
                                                 "word uses A–Z letters only, no spaces inside a word, no punctuation, "
                                                 "no numbers, no commentary.\n"
                                                 "Topic area: %2\n"
                                                 "Each word length must be between %3 and %4 characters inclusive.")
                                                 .arg(n)
                                                 .arg(domainTopicEnglish)
                                                 .arg(lo)
                                                 .arg(hi);

    QJsonArray messages;
    messages.append(userMessage);

    QJsonObject root;
    root[QStringLiteral("model")] = QStringLiteral("deepseek-chat");
    root[QStringLiteral("messages")] = messages;
    root[QStringLiteral("temperature")] = 0.55;
    root[QStringLiteral("max_tokens")] = qMin(512, 32 + n * 24);

    QJsonDocument doc(root);
    QNetworkRequest request(QUrl(QString::fromLatin1(kDeepSeekUrl)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.toUtf8());

    QNetworkReply* pReply = m_network->post(request, doc.toJson(QJsonDocument::Compact));
    connect(pReply, &QNetworkReply::finished, this, [this, pReply, lo, hi, n, domainTopicEnglish]() {
        pReply->deleteLater();
        if (pReply->error() != QNetworkReply::NoError) {
            emit rewardDiagnosticLine(
                QStringLiteral("reward_llm http_error code=%1 msg=%2 topic=%3")
                    .arg(static_cast<int>(pReply->error()))
                    .arg(truncateForLog(pReply->errorString(), 160))
                    .arg(truncateForLog(domainTopicEnglish, 80)));
            emitLocalFallbackBatch(lo, hi, n, QStringLiteral("after_http_error"),
                                   pReply->errorString());
            return;
        }

        const QJsonDocument responseDoc = QJsonDocument::fromJson(pReply->readAll());
        const QJsonObject responseObj = responseDoc.object();
        const QJsonArray choices = responseObj.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            emit rewardDiagnosticLine(QStringLiteral("reward_llm http_empty_choices topic=%1")
                                          .arg(truncateForLog(domainTopicEnglish, 120)));
            emitLocalFallbackBatch(lo, hi, n, QStringLiteral("empty_choices"));
            return;
        }
        const QJsonObject first = choices.first().toObject();
        const QJsonObject message = first.value(QStringLiteral("message")).toObject();
        const QString content = message.value(QStringLiteral("content")).toString();
        QStringList words = extractWordsFromModelContent(content.trimmed(), lo, hi, n);
        const int parsedModel = words.size();
        while (words.size() < n) {
            QString w;
            for (int attempt = 0; attempt < 40; ++attempt) {
                w = pickLocalWord(lo, hi);
                if (!words.contains(w)) {
                    break;
                }
            }
            words.append(w);
        }
        const int paddedLocal = n - parsedModel;
        const QString sample = words.mid(0, qMin(8, words.size())).join(QLatin1Char(','));
        emit rewardDiagnosticLine(QStringLiteral("reward_llm source=http_ok batch=%1 parsed_model=%2 padded_local=%3 "
                                                 "topic=%4 sample=%5")
                                      .arg(n)
                                      .arg(parsedModel)
                                      .arg(paddedLocal)
                                      .arg(truncateForLog(domainTopicEnglish, 80))
                                      .arg(sample));
        emit wordsBatchReady(words);
    });
}
