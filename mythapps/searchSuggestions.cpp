#include "searchSuggestions.h"

// QT headers
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>
#include <QXmlStreamReader>

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "FunctionLogger.h"

SearchSuggestions::SearchSuggestions() {
    LOGS(0, "");
    netRequestSearchSuggestions = new NetRequest("", "", "", "", false);

    // Is there a custom search suggest url?
    QString url = gCoreContext->GetSetting("MythAppsCustomSearchSuggestUrl");

    if (url.isEmpty()) {
        customSearchSuggestUrl = "https://en.wikipedia.org/w/api.php?action=opensearch&format=xml&search=";
    } else {
        customSearchSuggestUrl = url; // Example Format: search?output=toolbar&hl=en&ds=yt&q=
    }
}

/** \brief get a list of search suggestions
 *  \param search search query
 *  \return list of search suggestions */
QStringList SearchSuggestions::getSuggestions(QString search) {
    LOGS(0, "", "search", search);
    QStringList suggestions;
    QString response = netRequestSearchSuggestions->requestUrlPublic(customSearchSuggestUrl + search, "");

    QXmlStreamReader xml(response);

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement)
            if (xml.name() == "suggestion") {
                QStringRef str = xml.attributes().value("data");
                suggestions.append(str.toString());
            }

        if (xml.name() == "Text") {
            QString str = xml.readElementText();
            suggestions.append(str);
        }
        if (suggestions.size() == 9) {
            return suggestions;
        }
    }

    return suggestions;
}
