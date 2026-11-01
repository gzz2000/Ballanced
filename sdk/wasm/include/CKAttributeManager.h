#ifndef CKATTRIBUTEMANAGER_H
#define CKATTRIBUTEMANAGER_H

#include "CKDefines.h"
#include "CKBaseManager.h"
#include "XObjectArray.h"

/****************************************************************
Summary: Function called when an attribute is set or removed on an object.

Remarks:
    + A callback function can be set for a given attribute type with CKAttributeManager::SetAttributeCallbackFunction.
    + This function will be called each time an attribute with the given type is set (with Set = TRUE) or removed (with Set = FALSE)
    on an object.
See Also:CKAttributeManager::SetAttributeCallbackFunction,Using Attributes
*****************************************************/
typedef void (*CKATTRIBUTECALLBACK)(CKAttributeType AttribType, CKBOOL Set, CKBeObject *obj, void *arg);

struct CKAttributeDesc
{
    char Name[64];
    CKGUID ParameterType;
    XObjectPointerArray GlobalAttributeList;
    XObjectPointerArray AttributeList;
    CKAttributeCategory AttributeCategory;
    CK_CLASSID CompatibleCid;
    CKATTRIBUTECALLBACK CallbackFct;
    void *CallbackArg;
    char *DefaultValue;
    CKDWORD Flags;
    CKPluginEntry *CreatorDll;
};

struct CKAttributeCategoryDesc
{
  char *Name;
  CKDWORD Flags;
};


/*********************************************************************
Name: CKAttributeManager


Summary: Object attributes management.
Remarks:
+ Every CKBeObject may be given some attributes that behaviors or other
classes may ask for information.
+ An attribute is defined by :
    + A Name
    + A Category (Optional)
    + A Parameter type (Optional)
+ New types of attributes may be registered by giving them a name and the type of parameters that will come along with them. Once registered this
new type of attribute may be accessed by name or by the unique index returned by RegisterNewAttributeType.
+ Retrieving attributes by index is of course far more efficient than by Name.
+ The unique instance of CKAttributeManager can be retrieved by calling CKContext::GetAttributeManager() and its manager GUID is ATTRIBUTE_MANAGER_GUID.
+ See the Using attributes paper for more detail on how to use attributes.
See also: CKBeObject::SetAttribute,CKBeObject,CKParameterManager,Using Attributes
***********************************************************************/
class CKAttributeManager : public CKBaseManager
{
    friend class CKPluginManager;
    friend class CKFile;

public:
    //-------------------------------------------------------------------
    // Registration
    DLL_EXPORT CKAttributeType RegisterNewAttributeType(CKSTRING Name, CKGUID ParameterType, CK_CLASSID CompatibleCid = CKCID_BEOBJECT, CK_ATTRIBUT_FLAGS flags = CK_ATTRIBUT_SYSTEM);
    DLL_EXPORT void UnRegisterAttribute(CKAttributeType AttribType);
    DLL_EXPORT void UnRegisterAttribute(CKSTRING atname);

    DLL_EXPORT CKSTRING GetAttributeNameByType(CKAttributeType AttribType);
    DLL_EXPORT CKAttributeType GetAttributeTypeByName(CKSTRING AttribName);

    DLL_EXPORT void SetAttributeNameByType(CKAttributeType AttribType, CKSTRING name);

    DLL_EXPORT int GetAttributeCount();

    //-------------------------------------------------------------------
    // Attribute Parameter
    DLL_EXPORT CKGUID GetAttributeParameterGUID(CKAttributeType AttribType);
    DLL_EXPORT CKParameterType GetAttributeParameterType(CKAttributeType AttribType);

    //-------------------------------------------------------------------
    // Attribute Compatible Class Id
    DLL_EXPORT CK_CLASSID GetAttributeCompatibleClassId(CKAttributeType AttribType);

    //-------------------------------------------------------------------
    // Attribute Flags
    DLL_EXPORT CKBOOL IsAttributeIndexValid(CKAttributeType index);
    DLL_EXPORT CKBOOL IsCategoryIndexValid(CKAttributeCategory index);

    //-------------------------------------------------------------------
    // Attribute Flags
    DLL_EXPORT CK_ATTRIBUT_FLAGS GetAttributeFlags(CKAttributeType AttribType);

    //-------------------------------------------------------------------
    // Attribute Callback
    DLL_EXPORT void SetAttributeCallbackFunction(CKAttributeType AttribType, CKATTRIBUTECALLBACK fct, void *arg);

    //-------------------------------------------------------------------
    // Attribute Default Value
    DLL_EXPORT void SetAttributeDefaultValue(CKAttributeType AttribType, CKSTRING DefaultVal);
    DLL_EXPORT CKSTRING GetAttributeDefaultValue(CKAttributeType AttribType);

    //-------------------------------------------------------------------
    // Group By Attributes
    DLL_EXPORT const XObjectPointerArray &GetAttributeListPtr(CKAttributeType AttribType);
    DLL_EXPORT const XObjectPointerArray &GetGlobalAttributeListPtr(CKAttributeType AttribType);

    DLL_EXPORT const XObjectPointerArray &FillListByAttributes(CKAttributeType *ListAttrib, int AttribCount);
    DLL_EXPORT const XObjectPointerArray &FillListByGlobalAttributes(CKAttributeType *ListAttrib, int AttribCount);

    //----------------------------------------------------------------
    // Categories...
    DLL_EXPORT int GetCategoriesCount();
    DLL_EXPORT CKSTRING GetCategoryName(CKAttributeCategory index);
    DLL_EXPORT CKAttributeCategory GetCategoryByName(CKSTRING Name);

    DLL_EXPORT void SetCategoryName(CKAttributeCategory catType, CKSTRING name);

    DLL_EXPORT CKAttributeCategory AddCategory(CKSTRING Category, CKDWORD flags = 0);
    DLL_EXPORT void RemoveCategory(CKSTRING Category);

    DLL_EXPORT CKDWORD GetCategoryFlags(CKAttributeCategory cat);
    DLL_EXPORT CKDWORD GetCategoryFlags(CKSTRING cat);

    //-----------------------------------------------------------------
    // Attribute Category
    DLL_EXPORT void SetAttributeCategory(CKAttributeType AttribType, CKSTRING Category);
    DLL_EXPORT CKSTRING GetAttributeCategory(CKAttributeType AttribType);
    DLL_EXPORT CKAttributeCategory GetAttributeCategoryIndex(CKAttributeType AttribType);

    DLL_EXPORT void AddAttributeToObject(CKAttributeType AttribType, CKBeObject *beo);
    DLL_EXPORT void RemoveAttributeFromObject(CKAttributeType AttribType, CKBeObject *beo);

    DLL_EXPORT void RefreshList(CKObject *obj, CKScene *scene);

    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    // Internal functions
    //-------------------------------------------------------------------
    //-------------------------------------------------------------------

    DLL_EXPORT CKPluginEntry *GetCreatorDll(int pos);

    DLL_EXPORT void NewActiveScene(CKScene *scene);

    DLL_EXPORT void ObjectAddedToScene(CKBeObject *beo, CKScene *scene);
    DLL_EXPORT void ObjectRemovedFromScene(CKBeObject *beo, CKScene *scene);

    DLL_EXPORT void PatchRemapBeObjectFileChunk(CKStateChunk *chunk);
    DLL_EXPORT void PatchRemapBeObjectStateChunk(CKStateChunk *chunk);

    CKAttributeManager(CKContext *Context);

    virtual ~CKAttributeManager();

    virtual CKERROR PreClearAll();

    virtual CKERROR PostLoad();

    virtual CKERROR LoadData(CKStateChunk *chunk, CKFile *LoadedFile);

    virtual CKStateChunk *SaveData(CKFile *SavedFile);

    virtual CKERROR SequenceAddedToScene(CKScene *scn, CK_ID *objid, int count);

    virtual CKERROR SequenceRemovedFromScene(CKScene *scn, CK_ID *objid, int count);

    virtual CKDWORD GetValidFunctionsMask() { return CKMANAGER_FUNC_PreClearAll |
                                                     CKMANAGER_FUNC_PostLoad |
                                                     CKMANAGER_FUNC_OnSequenceAddedToScene |
                                                     CKMANAGER_FUNC_OnSequenceRemovedFromScene; }

    int m_AttributeInfoCount;
    CKAttributeDesc **m_AttributeInfos;
    int m_AttributeCategoryCount;
    CKAttributeCategoryDesc **m_AttributeCategories;
    int *m_ConversionTable;
    int m_ConversionTableCount;
    XBitArray m_AttributeMask;
    CKBOOL m_Saving;
    XObjectPointerArray m_AttributeList;
};

#endif // CKATTRIBUTEMANAGER_H