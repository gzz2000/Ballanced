#ifndef CKOBJECTDECLARATION_H
#define CKOBJECTDECLARATION_H

#include "CKObject.h"

/*****************************************************************************
Summary: External object declaration

Remarks:
    + A CKObjectDeclaration is used by Virtools engine to store a list of available extensions objects.
    + Only declarations of CKBehaviorPrototype are supported in this version. The CKObjectDeclaration contains
    a short description about the behavior prototype and its author and also contains a pointer to the function
    that will be used to create the behavior prototype when needed.

    + A CKObjectDeclaration is created with CreateCKObjectDeclaration.



Example:
    // The following sample creates an object declaration for the "Rotate" behavior.
    // This object declaration is given :
    //	- The type of object declaration (Must Be CKDLL_BEHAVIORPROTOTYPE)
    //	- A short description of what the behavior is supposed to do.
    //	- The category in which this behavior will appear in the Virtools interface.
    //	- A unique CKGUID
    //	- Author and Version info
    //	- The class identifier of objects to which the behavior can be applied to.
    //	- The function that will create the CKBehaviorPrototype for this behavior.

    CKObjectDeclaration	*FillBehaviorRotateDecl()
    {
        CKObjectDeclaration *od = CreateCKObjectDeclaration("Rotate");

        od->SetType(CKDLL_BEHAVIORPROTOTYPE);
        od->SetDescription("Rotates the 3D Entity.");
        od->SetCategory("3D Transformations/Basic");
        od->SetGuid(CKGUID(0xffffffee, 0xeeffffff));
        od->SetAuthorGuid(VIRTOOLS_GUID);
        od->SetAuthorName("Virtools");
        od->SetVersion(0x00010000);
        od->SetCompatibleClassId(CKCID_3DENTITY);

        od->SetCreationFunction(CreateRotateProto);
        return od;
    }


See also: CKBehaviorPrototype,
********************************************************************************/
class CKObjectDeclaration
{
public:
    //////////////////////////////////////////////////
    ////	CKObjectDeclaration Member Functions   ///
    //////////////////////////////////////////////////

    //-----------------------------------------------------
    // Description
    DLL_EXPORT void SetDescription(CKSTRING Description);
    DLL_EXPORT CKSTRING GetDescription();

    //-----------------------------------------------------
    // Behavior GUID
    DLL_EXPORT void SetGuid(CKGUID guid);
    DLL_EXPORT CKGUID GetGuid();

    DLL_EXPORT void SetType(int type);

    DLL_EXPORT int GetType();

    //-----------------------------------------------------
    // Dependencie on Managers
    DLL_EXPORT void NeedManager(CKGUID Manager);

    //-----------------------------------------------------
    // Creation function (function that will create the prototype )
    DLL_EXPORT void SetCreationFunction(CKDLL_CREATEPROTOFUNCTION f);
    DLL_EXPORT CKDLL_CREATEPROTOFUNCTION GetCreationFunction();

    //-----------------------------------------------------
    // Author information
    DLL_EXPORT void SetAuthorGuid(CKGUID guid);
    DLL_EXPORT CKGUID GetAuthorGuid();

    DLL_EXPORT void SetAuthorName(CKSTRING Name);
    DLL_EXPORT CKSTRING GetAuthorName();

    //-----------------------------------------------------
    // Version information
    DLL_EXPORT void SetVersion(CKDWORD verion);
    DLL_EXPORT CKDWORD GetVersion();

    //-----------------------------------------------------
    // Class Id of object to which the declared behavior can apply
    DLL_EXPORT void SetCompatibleClassId(CK_CLASSID id);
    DLL_EXPORT CK_CLASSID GetCompatibleClassId();

    //-----------------------------------------------------
    // Category in which the behavior will be presented
    DLL_EXPORT void SetCategory(CKSTRING cat);
    DLL_EXPORT CKSTRING GetCategory();

    /*************************************************
    Summary: Gets the name of behavior Prototype.
    Return Value:
        A pointer to Behavious Prototype name.
    Remarks:
        + The object declaration name is always the same
        than the behavior prototype.
    ********************************************/
    CKSTRING GetName() { return m_Name.Str(); }

    /**************************************************
    Summary:Returns the index of the DLL that has declared this object.
    Return Value:
        Index of the plugin that registered this object in the plugin manager.
    Remarks:
    + The return value is the index of the plugin in the behavior
    category of the plugin manager.
    + To retrieve information about the plugin use :

    CKPluginManager::GetPluginDllInfo(CKPLUGIN_BEHAVIOR_DLL,Index);

    See Also:CKPluginManager
    ****************************************************/
    int GetPluginIndex() { return m_PluginIndex; }

    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    // Internal functions
    //-------------------------------------------------------------------
    //-------------------------------------------------------------------

    CKBehaviorPrototype *GetProto() { return m_Proto; }
    void SetProto(CKBehaviorPrototype *proto) { m_Proto = proto; }

    CKObjectDeclaration(CKSTRING Name);
    virtual ~CKObjectDeclaration();
    void SetPluginIndex(int Index)
    {
        m_PluginIndex = Index;
    }
    int GetManagerNeededCount() { return m_ManagersGuid.Size(); }
    CKGUID GetManagerNeeded(int Index) { return m_ManagersGuid[Index]; }

    CKGUID m_Guid;
    CK_CLASSID m_CompatibleClassID;
    CKDLL_CREATEPROTOFUNCTION m_CreationFunction;
    CKDWORD m_Version;
    CKSTRING m_Description;
    CKBehaviorPrototype *m_Proto;
    int m_Type;
    CKGUID m_AuthorGuid;
    CKSTRING m_AuthorName;
    CKSTRING m_Category;
    XString m_Name;
    int m_PluginIndex;
    XArray<CKGUID> m_ManagersGuid;
};

#endif // CKOBJECTDECLARATION_H
