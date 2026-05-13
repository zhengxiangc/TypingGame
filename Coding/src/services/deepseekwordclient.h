/* -------------------------------------------------------------------------
//  文件名      :  deepseekwordclient.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTDeepSeekWordClient (async Deepseek word batches + local fallback).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_DEEPSEEKWORDCLIENT_H__
#define __CLASSEXAM_DEEPSEEKWORDCLIENT_H__

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;

// Asynchronously requests English words from Deepseek chat API (batch in one HTTP call).
// API key: environment variable Deepseek_Api only (system/user env; not read from UI or QSettings).
// On network or parse failure, pads with words from a local fallback list (never emits empty).
class KCTDeepSeekWordClient : public QObject
{
    Q_OBJECT

public:
    explicit KCTDeepSeekWordClient(QObject* parent = nullptr);

    // One request; emits wordsBatchReady with up to batchSize words (may pad with local fallback).
    void requestWordsBatchAsync(int minLength, int maxLength, int batchSize, const QString& domainTopicEnglish);

signals:
    void wordsBatchReady(const QStringList& wordsUppercase);
    // Single-line diagnostics for logging (HTTP vs local, parse counts).
    void rewardDiagnosticLine(const QString& line);

private:
    void emitLocalFallbackBatch(int minLength, int maxLength, int batchSize, const QString& reason,
                                const QString& detail = QString());
    static QString sanitizeToSingleWord(const QString& raw, int minLength, int maxLength);
    static QStringList extractWordsFromModelContent(const QString& content, int minLength, int maxLength,
                                                    int maxCount);
    static QString pickLocalWord(int minLength, int maxLength);

    QNetworkAccessManager* m_network = nullptr;

    KCTDeepSeekWordClient(const KCTDeepSeekWordClient&) = delete;
    KCTDeepSeekWordClient& operator=(const KCTDeepSeekWordClient&) = delete;
};

#endif // __CLASSEXAM_DEEPSEEKWORDCLIENT_H__
