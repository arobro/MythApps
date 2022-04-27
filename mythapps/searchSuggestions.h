#ifndef SearchSuggestions_h
#define SearchSuggestions_h

#include "netRequest.h"
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>

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
#endif
