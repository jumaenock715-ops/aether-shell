pragma Singleton
import QtQuick

QtObject {
    readonly property var list: [
        { appId: "files",    name: "Files",    tint: "#6D4AFF" },
        { appId: "browser",  name: "Browser",  tint: "#14B8D4" },
        { appId: "android",  name: "Android",  tint: "#3DDC97" },
        { appId: "terminal", name: "Terminal", tint: "#FFB020" },
        { appId: "media",    name: "Media",    tint: "#FF5C7A" },
        { appId: "notes",    name: "Notes",    tint: "#8B9BFF" },
        { appId: "settings", name: "Settings", tint: "#7A8499" }
    ]

    function byId(appId) {
        for (var i = 0; i < list.length; i++)
            if (list[i].appId === appId) return list[i]
        return null
    }
}
