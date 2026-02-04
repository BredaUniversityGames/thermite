[1mdiff --git a/editor/imgui/types/tmt.hpp b/editor/imgui/types/tmt.hpp[m
[1mindex f150f8a..ff5a994 100644[m
[1m--- a/editor/imgui/types/tmt.hpp[m
[1m+++ b/editor/imgui/types/tmt.hpp[m
[36m@@ -3,6 +3,8 @@[m
 #include <ImReflect.hpp>[m
 #include <engine/core/resources.hpp>[m
 [m
[32m+[m[32m#include "editor/windows/asset_browser.hpp"[m
[32m+[m
 template <typename T>[m
 inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef<T>& value, ImSettings& settings, ImResponse& response) {[m
     ImGui::BeginGroup();[m
[36m@@ -19,32 +21,18 @@[m [minline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::ResourceRef[m
     }[m
     ImGui::EndGroup();[m
 [m
[31m-    if (ImGui::BeginDragDropTarget()) {[m
[31m-        // Get the current payload to check if its a FileLocation.[m
[31m-        const ImGuiPayload* payload = ImGui::GetDragDropPayload();[m
[31m-        if (payload != nullptr && payload->IsDataType("FileLocation")) {[m
[31m-            const std::string_view json_string {static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize)};[m
[31m-            tmt::IO::FileLocation file_location;[m
[31m-            tmt::Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);[m
[31m-[m
[31m-            // Now that we know the user is dragging a FileLocation type, we can check its file extension to see if its valid for the resource type.[m
[31m-            const std::string& extension = file_location.relative_path.extension().generic_string();[m
[31m-            if (T::SUPPORTED_FILE_EXTENSIONS.contains(extension) && ImGui::AcceptDragDropPayload("FileLocation") != nullptr) {[m
[31m-                // If the file extension is valid start excepting the payload, this will return true once the user drops it.[m
[31m-                value.file_location = file_location;[m
[32m+[m[32m    if (auto drag_ctx = tmt::AssetBrowser::begin_file_location_drag()) {[m
[32m+[m[32m        if (T::SUPPORTED_FILE_EXTENSIONS.contains(drag_ctx.get_extension()) && drag_ctx.accept()) {[m
[32m+[m[32m            value.file_location = *drag_ctx.location;[m
 [m
[31m-                // TODO: Sven move this if-check to resources.hpp[m
[31m-                if constexpr (std::derived_from<T, tmt::FileResource>) {[m
[31m-                    value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;[m
[31m-                } else if constexpr (requires { typename T::is_runtime_resource; }) {[m
[31m-                    value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;[m
[31m-                } else {[m
[31m-                    static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");[m
[31m-                }[m
[32m+[m[32m            if constexpr (std::derived_from<T, tmt::FileResource>) {[m
[32m+[m[32m                value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;[m
[32m+[m[32m            } else if constexpr (requires { typename T::is_runtime_resource; }) {[m
[32m+[m[32m                value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;[m
[32m+[m[32m            } else {[m
[32m+[m[32m                static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T>");[m
             }[m
         }[m
[31m-[m
[31m-        ImGui::EndDragDropTarget();[m
     }[m
 [m
     if constexpr (ImReflect::Detail::has_imreflect_input_v<T>) {[m
[1mdiff --git a/editor/windows/asset_browser.cpp b/editor/windows/asset_browser.cpp[m
[1mindex d6fcb4f..c6c7aee 100644[m
[1m--- a/editor/windows/asset_browser.cpp[m
[1m+++ b/editor/windows/asset_browser.cpp[m
[36m@@ -284,6 +284,33 @@[m [mvoid AssetBrowser::import_assets_dialog() const {[m
     );[m
 }[m
 [m
[32m+[m[32mFileLocationDragContext AssetBrowser::begin_file_location_drag() {[m
[32m+[m[32m    FileLocationDragContext context;[m
[32m+[m
[32m+[m[32m    if (!ImGui::BeginDragDropTarget()) {[m
[32m+[m[32m        return context;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    context.is_active = true;[m
[32m+[m
[32m+[m[32m    const ImGuiPayload* payload = ImGui::GetDragDropPayload();[m
[32m+[m[32m    if (payload == nullptr || !payload->IsDataType("FileLocation")) {[m
[32m+[m[32m        return context;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    try {[m
[32m+[m[32m        const std::string_view json_string {static_cast<char*>(payload->Data), static_cast<size_t>(payload->DataSize)};[m
[32m+[m
[32m+[m[32m        IO::FileLocation file_location;[m
[32m+[m[32m        Serializer::deserialize(nlohmann::ordered_json::parse(json_string), file_location);[m
[32m+[m[32m        context.location = std::move(file_location);[m
[32m+[m[32m    } catch (const std::exception& e) {[m
[32m+[m[32m        Log::error("Failed to deserialize FileLocation payload: {}", e.what());[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    return context;[m
[32m+[m[32m}[m
[32m+[m
 void AssetBrowser::update_location_history() {[m
     if (pending_viewing_location.has_value()) {[m
         // Clear the redo history if the user manually went to a new directory.[m
[1mdiff --git a/editor/windows/asset_browser.hpp b/editor/windows/asset_browser.hpp[m
[1mindex 6ffe359..d3485a6 100644[m
[1m--- a/editor/windows/asset_browser.hpp[m
[1m+++ b/editor/windows/asset_browser.hpp[m
[36m@@ -10,6 +10,29 @@[m
 [m
 namespace tmt {[m
 [m
[32m+[m[32mstruct FileLocationDragContext {[m
[32m+[m[32m    std::optional<IO::FileLocation> location;[m
[32m+[m
[32m+[m[32m    ~FileLocationDragContext() {[m
[32m+[m[32m        if (is_active) {[m
[32m+[m[32m            ImGui::EndDragDropTarget();[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool accept() {[m
[32m+[m[32m        if (!is_active || !location.has_value()) return false;[m
[32m+[m[32m        return ImGui::AcceptDragDropPayload("FileLocation") != nullptr;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    explicit operator bool() const { return location.has_value(); }[m
[32m+[m
[32m+[m[32m    std::string get_extension() const { return location ? location->relative_path.extension().generic_string() : ""; }[m
[32m+[m
[32m+[m[32m   private:[m
[32m+[m[32m    bool is_active = false;[m
[32m+[m[32m    friend class AssetBrowser;[m
[32m+[m[32m};[m
[32m+[m
 class AssetBrowser : public internal::OnSdlEvent, public IWindow {[m
    public:[m
     constexpr std::string get_title() const override { return ICON_MS_FOLDER " Asset Browser"; }[m
[36m@@ -28,6 +51,8 @@[m [mclass AssetBrowser : public internal::OnSdlEvent, public IWindow {[m
     static void import_asset(const IO::FileLocation& import_file, const IO::FileLocation& location);[m
     void import_assets_dialog() const;[m
 [m
[32m+[m[32m    static FileLocationDragContext begin_file_location_drag();[m
[32m+[m
    private:[m
     struct Directory {[m
         IO::FileLocation location;[m
[1mdiff --git a/extern/JsonReflect b/extern/JsonReflect[m
[1mindex 85f7629..f1ee0d3 160000[m
[1m--- a/extern/JsonReflect[m
[1m+++ b/extern/JsonReflect[m
[36m@@ -1 +1 @@[m
[31m-Subproject commit 85f7629228675417f2d24dee8d31a7b1b49cf904[m
[32m+[m[32mSubproject commit f1ee0d3221cadfadaed4994985f6cf40680ba12f[m
[1mdiff --git a/projects/game/assets/scenes/DragonScene.scene b/projects/game/assets/scenes/DragonScene.scene[m
[1mindex 41424bd..e69de29 100644[m
[1m--- a/projects/game/assets/scenes/DragonScene.scene[m
[1m+++ b/projects/game/assets/scenes/DragonScene.scene[m
[36m@@ -1,102 +0,0 @@[m
[31m-{[m
[31m-    "version": 1,[m
[31m-    "entities": [[m
[31m-        2,[m
[31m-        3,[m
[31m-        4,[m
[31m-        5[m
[31m-    ],[m
[31m-    "Name": {[m
[31m-        "2": {[m
[31m-            "diff": [[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/name",[m
[31m-                    "value": "Car 2"[m
[31m-                }[m
[31m-            ][m
[31m-        }[m
[31m-    },[m
[31m-    "Transform": {[m
[31m-        "2": {[m
[31m-            "diff": [[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/local_position/y",[m
[31m-                    "value": -2.647066593170166[m
[31m-                }[m
[31m-            ][m
[31m-        },[m
[31m-        "3": {[m
[31m-            "diff": [[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/children/0",[m
[31m-                    "value": 4[m
[31m-                },[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/children/1",[m
[31m-                    "value": 5[m
[31m-                }[m
[31m-            ][m
[31m-        },[m
[31m-        "4": {[m
[31m-            "diff": [[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/parent",[m
[31m-                    "value": 3[m
[31m-                }[m
[31m-            ][m
[31m-        },[m
[31m-        "5": {[m
[31m-            "diff": [[m
[31m-                {[m
[31m-                    "op": "replace",[m
[31m-                    "path": "/parent",[m
[31m-                    "value": 3[m
[31m-                }[m
[31m-            ][m
[31m-        }[m
[31m-    },[m
[31m-    "Prefab": {[m
[31m-        "2": {[m
[31m-            "source_location": {[m
[31m-                "sub_location": "PROJECT",[m
[31m-                "relative_path": "Car.prefab"[m
[31m-            },[m
[31m-            "source_entity": 2,[m
[31m-            "root_entity": 2,[m
[31m-            "instance_id": "e8cc1aaf-7dd9-471d-a1cc-e493b0d5ee87"[m
[31m-        },[m
[31m-        "3": {[m
[31m-            "source_location": {[m
[31m-                "sub_location": "PROJECT",[m
[31m-                "relative_path": "Car.prefab"[m
[31m-            },[m
[31m-            "source_entity": 2,[m
[31m-            "root_entity": 3,[m
[31m-            "instance_id": "e7cc1aaf-7dd9-471d-a1cc-e493b0d5ee87"[m
[31m-        },[m
[31m-        "4": {[m
[31m-            "source_location": {[m
[31m-                "sub_location": "PROJECT",[m
[31m-                "relative_path": "Car.prefab"[m
[31m-            },[m
[31m-            "source_entity": 0,[m
[31m-            "root_entity": 3,[m
[31m-            "instance_id": "e7cc1aaf-7dd9-471d-a1cc-e493b0d5ee87"[m
[31m-        },[m
[31m-        "5": {[m
[31m-            "source_location": {[m
[31m-                "sub_location": "PROJECT",[m
[31m-                "relative_path": "Car.prefab"[m
[31m-            },[m
[31m-            "source_entity": 1048577,[m
[31m-            "root_entity": 3,[m
[31m-            "instance_id": "e7cc1aaf-7dd9-471d-a1cc-e493b0d5ee87"[m
[31m-        }[m
[31m-    },[m
[31m-    "systems": null[m
[31m-}[m
\ No newline at end of file[m
