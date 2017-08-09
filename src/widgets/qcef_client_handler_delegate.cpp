// Copyright (c) 2017 LiuLang. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "widgets/qcef_client_handler_delegate.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/qcef_web_channel_consts.h"

QCefClientHandlerDelegate::QCefClientHandlerDelegate(QCefWebPage* web_page)
  : web_page_(web_page){
}

QCefClientHandlerDelegate::~QCefClientHandlerDelegate() {
  if (cef_browser_ != nullptr) {
    cef_browser_->GetHost()->CloseBrowser(true);
    cef_browser_->Release();
    cef_browser_ = nullptr;
  }
}

void QCefClientHandlerDelegate::OnBeforePopup(const CefString& target_url) {
  // TODO(LiuLang): Add option.
  if (cef_browser_ != nullptr) {
    cef_browser_->GetMainFrame()->LoadURL(target_url);
  }
}

void QCefClientHandlerDelegate::OnBrowserCreated(
    CefRefPtr<CefBrowser> browser) {
  if (cef_browser_ == nullptr) {
    cef_browser_ = browser;
  }
}

void QCefClientHandlerDelegate::OnBeforeClose() {
  cef_browser_ = nullptr;
  // TODO(LiuLang): Emit close signal.
}

void QCefClientHandlerDelegate::OnFaviconURLChange(
    const std::vector<CefString>& urls) {
  if (urls.size() > 0) {
    const QString url = QString::fromStdString(urls[0]);
    emit web_page_->iconUrlChanged(url);
  }
}

void QCefClientHandlerDelegate::OnLoadStarted() {
  emit web_page_->loadStarted();
}

void QCefClientHandlerDelegate::OnLoadEnd(int httpStatusCode) {
  Q_UNUSED(httpStatusCode);
  emit web_page_->loadFinished(true);
}

std::string QCefClientHandlerDelegate::OnLoadError(int errorCode) {
  Q_UNUSED(errorCode);
  emit web_page_->loadFinished(false);
  // TODO(LiuLang): Pass |errorCode|.
  return web_page_->pageErrorContent().toStdString();
}

bool QCefClientHandlerDelegate::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  Q_UNUSED(browser);
  Q_UNUSED(source_process);
  const std::string name(message->GetName());
  if (name == kQCefRenderContextCreated) {
    web_page_->createTransportChannel();
    return true;
  }
  if (name == kQCefRenderContextReleased) {
    web_page_->releaseTransportChannel();
    return true;
  }
  if (name == kQCefRenderQtMessage) {
    CefRefPtr<CefListValue> args = message->GetArgumentList();
    const QString msg = QString::fromStdString(args->GetString(0));
    const QJsonDocument doc(QJsonDocument::fromJson(msg.toUtf8()));
    if (doc.isObject()) {
      web_page_->handleWebMessage(doc.object());
    } else {
      qWarning() << "QCefClientHandlerDelegate::OnProcessMessageReceived()"
                 << "invalid json message:" << msg;
    }
    return true;
  }

  return false;
}

void QCefClientHandlerDelegate::OnSetFullscreen(bool fullscreen) {
  emit web_page_->fullscreenRequested(fullscreen);
}

void QCefClientHandlerDelegate::OnTitleChanged(const CefString& title) {
  emit web_page_->titleChanged(QString::fromStdString(title));
}

void QCefClientHandlerDelegate::OnUrlChanged(const CefString& url) {
  emit web_page_->urlChanged(QUrl(QString::fromStdString(url)));
}
