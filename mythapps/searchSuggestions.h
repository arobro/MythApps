#ifndef SearchSuggestions_h
#define SearchSuggestions_h

// QT headers
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>

// MythApps headers
#include "netRequest.h"

#endif

/** \class SearchSuggestions
 *  \brief Creates a list of search suggestions */
class SearchSuggestions {
  public:
    SearchSuggestions();
    QStringList getSuggestions(QString search);

  private:
    NetRequest *netRequestSearchSuggestions;
    QString customSearchSuggestUrl;
};
