pragma Singleton
import QtQuick

QtObject {
    readonly property var list: [
        { appId: "files", kind: "builtin",    name: "Files",    tint: "#6D4AFF" },
        { appId: "browser", kind: "builtin",  name: "Browser",  tint: "#14B8D4" },
        { appId: "android", kind: "builtin",  name: "Android",  tint: "#3DDC97" },
        { appId: "terminal", kind: "builtin", name: "Terminal", tint: "#FFB020" },
        { appId: "media", kind: "builtin",    name: "Media",    tint: "#FF5C7A" },
        { appId: "notes", kind: "builtin",    name: "Notes",    tint: "#8B9BFF" },
        { appId: "settings", kind: "builtin", name: "Settings", tint: "#7A8499" }
    ]

    function byId(appId) {
        for (var i = 0; i < list.length; i++)
            if (list[i].appId === appId) return list[i]
        return null
    }
}
