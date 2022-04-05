#pragma once

#pragma warning(push)
#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <RE/T/TESForm.h>
#include <RE/T/TESObjectREFR.h>
#pragma warning(pop)

#include "BindingDefinition.h"

using namespace NoESP;
using namespace RE::BSScript;
using namespace RE::BSScript::Internal;

namespace NoESP::PapyrusScriptBindings {

    void AutoFillProperties(const RE::BSTSmartPointer<RE::BSScript::Object>& object) {
//        if (true) return; // Come back to this...

        auto* typeInfo = object->GetTypeInfo();
        auto* properties = typeInfo->GetPropertyIter();
        for (uint32_t i = 0; i < typeInfo->propertyCount; i++) {
            auto propertyName = properties[i].name;
            Log("Getting type info about property '{}'", propertyName.c_str());
            auto* propertyTypeInfo = properties[i].info.type.GetTypeInfo();
            auto type = propertyTypeInfo->GetRawType();
            if (type == TypeInfo::RawType::kObject) {
                Log("Property {} is OBJECT", propertyName.c_str());
                Log("The property object is of type {}", propertyTypeInfo->name.c_str());
            } else if (type == TypeInfo::RawType::kString) {
                Log("Property {} is String", propertyName.c_str());
            } else if (type == TypeInfo::RawType::kInt) {
                Log("Property {} is Int", propertyName.c_str());
            } else {
                auto typeId = (size_t) type;
                Log("Dunno prop type {}? Let's see here...", typeId);
                Log("Dunno prop type NAME {}? Let's see here...", propertyName.c_str());
                Log("The property object is of type {}", propertyTypeInfo->name.c_str());
            }

//            if (properties[i].info.getFunction) {
//                Log("THERE IS A GETTER FUNCTION");
//            } else {
//                Log("there is NO getter function");
//            }

            // propertyTypeInfo->GetRawType()
//             TypeInfo::RawType::


//            Log("here we go...");
//            auto typeName = propertyTypeInfo->name;
//            Log("The type name is: '{}'", typeName.c_str());

//            auto* propertyVariable = object->GetProperty(propertyName);
//            if (propertyVariable->IsObject()) {
//                auto* form = RE::TESForm::LookupByEditorID(propertyName);
//                if (form) {
//                    auto* vm = VirtualMachine::GetSingleton();
//                    auto* handlePolicy = vm->GetObjectHandlePolicy();
//                    RE::VMHandle handle = handlePolicy->GetHandleForObject(form->GetFormType(), form);
//                    RE::BSTSmartPointer<RE::BSScript::Object> objectPtr;
//                    vm->CreateObject(typeName, objectPtr);
//                    auto* bindPolicy = vm->GetObjectBindPolicy();
//                    bindPolicy->BindObject(objectPtr, handle);
//                    propertyVariable->SetObject(objectPtr);
//                } else {
//                    // TODO - support all other property types! the primitives!
//                }
//            }
        }
    }

    void SetProperties(const RE::BSTSmartPointer<RE::BSScript::Object>& object, FormPropertyMap& propertyMap) {
        auto* typeInfo = object->GetTypeInfo();
        auto* properties = typeInfo->GetPropertyIter();
        for (uint32_t i = 0; i < typeInfo->propertyCount; i++) {
            auto typeName = properties[i].info.type.GetTypeInfo()->GetName();
            auto propertyName = properties[i].name;
            auto* propertyVariable = object->GetProperty(propertyName);
            if (propertyMap.contains(Utilities::ToLowerCase(propertyName.c_str()))) {

                // TODO make sure we do lowercase all the things!

                // TODO - caching!
                if (propertyVariable->IsString()) {
                    Log("OMG A STRING! '{}' => '{}'", propertyName.c_str(), propertyMap[std::string(propertyName.c_str())].PropertyValueText);
                } else {
                    Log("Unsupported property type for '{}'", propertyName.c_str());
                }
            }
        }
    }

    void BindToForm(std::string scriptName, const RE::TESForm& form, FormPropertyMap& propertiesToSet, bool addOnce = false) {
        bool autoFillProperties = true;
        if (scriptName.starts_with('!')) {
            autoFillProperties = false;
            scriptName = scriptName.substr(1); // Remove '!'
        }

        try {
            auto* vm = VirtualMachine::GetSingleton();
            auto* handlePolicy = vm->GetObjectHandlePolicy();
            RE::VMHandle handle = handlePolicy->GetHandleForObject(form.GetFormType(), (RE::TESForm*) &form);
            if (handle) {

                // If there is already a script with the same name attached to this object, don't bind a new one
                RE::BSFixedString caseInsensitiveScriptName = scriptName;
                if (addOnce) {
                    if (vm->attachedScripts.contains(handle)) {
                        for (auto& attachedScript : vm->attachedScripts.find(handle)->second) {
                            if (attachedScript->GetTypeInfo()->GetName() == caseInsensitiveScriptName) {
                                return; // Don't bind! Already bound!
                            }
                        }
                    }
                }

                RE::BSTSmartPointer<RE::BSScript::Object> objectPtr;
                vm->CreateObject(scriptName, objectPtr);
                auto* bindPolicy = vm->GetObjectBindPolicy();
                if (autoFillProperties) {
                    AutoFillProperties(objectPtr);
                }
//                if (def) {
//                    SetProperties(objectPtr, def->PropertyValues);
//                }
                try {
                    bindPolicy->BindObject(objectPtr, handle);
                } catch (...) {
                    Log("Failed to bind object to handle for '{}'", scriptName);
                    return;
                }

                auto* ref = form.AsReference();
                if (ref) {
                    auto* baseForm = ref->GetBaseObject();
                    Log("Bound script '{}' to reference '{}' 0x{:x} (base '{}' 0x{:x})!", scriptName, form.GetName(), form.formID, baseForm->GetName(), baseForm->formID);
                } else {
                    Log("Bound script '{}' to form '{}' 0x{:x}!", scriptName, form.GetName(), form.formID);
                }
            } else {
                Log("Error getting handle for script {} to reference", scriptName);
            }
        } catch (...) {
            Log("Error binding script {} to reference", scriptName);
        }
    }

    void BindToFormPointer(std::string scriptName, RE::TESForm* form, FormPropertyMap& propertiesToSet, bool addOnce = false) {
        if (! form) return;

        bool autoFillProperties = true;
        if (scriptName.starts_with('!')) {
            autoFillProperties = false;
            scriptName = scriptName.substr(1); // Remove '!'
        }

        try {
            auto* vm = VirtualMachine::GetSingleton();
            auto* handlePolicy = vm->GetObjectHandlePolicy();
            RE::VMHandle handle = handlePolicy->GetHandleForObject(form->GetFormType(), form);
            if (handle) {

                // If there is already a script with the same name attached to this object, don't bind a new one
                RE::BSFixedString caseInsensitiveScriptName = scriptName;
                if (addOnce) {
                    if (vm->attachedScripts.contains(handle)) {
                        for (auto& attachedScript : vm->attachedScripts.find(handle)->second) {
                            if (attachedScript->GetTypeInfo()->GetName() == caseInsensitiveScriptName) {
                                return; // Don't bind! Already bound!
                            }
                        }
                    }
                }

                RE::BSTSmartPointer<RE::BSScript::Object> objectPtr;
                vm->CreateObject(scriptName, objectPtr);
                auto* bindPolicy = vm->GetObjectBindPolicy();
                if (autoFillProperties) {
                    AutoFillProperties(objectPtr);
                }
                bindPolicy->BindObject(objectPtr, handle);

                auto* ref = form->AsReference();
                if (ref) {
                    auto* baseForm = ref->GetBaseObject();
                    Log("Bound script '{}' to reference '{}' 0x{:x} (base '{}' 0x{:x})!", scriptName, form->GetName(), form->formID, baseForm->GetName(), baseForm->formID);
                } else {
                    Log("Bound script '{}' to form '{}' 0x{:x}!", scriptName, form->GetName(), form->formID);
                }
            } else {
                Log("Error getting handle for script {} to reference", scriptName);
            }
        } catch (...) {
            Log("Error binding script {} to reference", scriptName);
        }
    }

    void BindToEditorId(const std::string& scriptName, const std::string& editorId, FormPropertyMap& propertiesToSet, bool addOnce = false) {
        auto* form = RE::TESForm::LookupByEditorID(editorId);
        if (form) {
            BindToFormPointer(scriptName, form, propertiesToSet, addOnce);
        } else {
            Log("Could not find Form via Editor ID: '{}' for script '{}'", editorId, scriptName);
        }
    }

    void BindToFormId(const std::string& scriptName, RE::FormID formId, FormPropertyMap& propertiesToSet, const std::string optionalPluginFile = "", bool addOnce = false) {
        Log("Bind script '{}' to form ID 0x{:x}", scriptName, formId);
        if (optionalPluginFile.empty()) {
            auto* form = RE::TESForm::LookupByID(formId);
            if (form) {
                BindToFormPointer(scriptName, form, propertiesToSet, addOnce);
            } else {
                Log("Could not find Form via Form ID: '{}' for script '{}'", formId, scriptName);
            }
        } else {
            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            if (dataHandler->GetModIndex(optionalPluginFile) != 255) {
                auto* form = dataHandler->LookupForm(formId, optionalPluginFile);
                if (form) {
                    BindToFormPointer(scriptName, form, propertiesToSet, addOnce);
                } else {
                    Log("Could not find Form via Form ID: '{}' in plugin '{}' for script '{}'", formId, optionalPluginFile, scriptName);
                }
            } else {
                Log("Could not find plugin '{}' for script '{}'", optionalPluginFile, scriptName);
            }
        }
    }

    void Bind(BindingDefinition& def) {
        try {
            if (def.Type == BindingDefinitionType::EditorID && def.EditorIdMatcher.Type == EditorIdMatcherType::Exact) {
                BindToEditorId(def.ScriptName, def.EditorIdMatcher.Text, def.PropertyValues, def.AddOnce);
            } else if (def.Type == BindingDefinitionType::FormID) {
                BindToFormId(def.ScriptName, def.FormID, def.PropertyValues, def.Plugin, def.AddOnce);
            }
        } catch (...) {
            if (def.Filename.empty()) {
                Log("Bind() error {} {} to {}", def.EditorIdMatcher.Text, def.FormID, def.ScriptName);
            } else {
                Log("Bind() error {}", def.Filename);
            }
        }
    }
}