/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* The long avoided variant support for xpcom. */

#ifndef nsVariant_h
#define nsVariant_h

#include "nsIVariant.h"
#include "nsStringFwd.h"
#include "xpt_struct.h"

class nsCycleCollectionTraversalCallback;

/** 
 * Map the nsAUTF8String, nsUTF8String classes to the nsACString and
 * nsCString classes respectively for now.  These defines need to be removed
 * once Jag lands his nsUTF8String implementation.
 */
#define nsAUTF8String nsACString
#define nsUTF8String nsCString
#define PromiseFlatUTF8String PromiseFlatCString

/**
 * nsDiscriminatedUnion is a type that nsIVariant implementors *may* use 
 * to hold underlying data. It has no methods. So, its use requires no linkage
 * to the xpcom module.
 */

struct nsDiscriminatedUnion
{
    union {
        PRInt8         mInt8Value;
        PRInt16        mInt16Value;
        PRInt32        mInt32Value;
        PRInt64        mInt64Value;
        PRUint8        mUint8Value;
        PRUint16       mUint16Value;
        PRUint32       mUint32Value;
        PRUint64       mUint64Value;
        float          mFloatValue;
        double         mDoubleValue;
        bool           mBoolValue;
        char           mCharValue;
        PRUnichar      mWCharValue;
        nsIID          mIDValue;
        nsAString*     mAStringValue;
        nsAUTF8String* mUTF8StringValue;
        nsACString*    mCStringValue;
        struct {
            nsISupports* mInterfaceValue;
            nsIID        mInterfaceID;
        } iface;
        struct {
            nsIID        mArrayInterfaceID;
            void*        mArrayValue;
            PRUint32     mArrayCount;
            PRUint16     mArrayType;
        } array;
        struct {
            char*        mStringValue;
            PRUint32     mStringLength;
        } str;
        struct {
            PRUnichar*   mWStringValue;
            PRUint32     mWStringLength;
        } wstr;
    } u;
    PRUint16       mType;
};

/**
 * nsVariant implements the generic variant support. The xpcom module registers
 * a factory (see NS_VARIANT_CONTRACTID in nsIVariant.idl) that will create 
 * these objects. They are created 'empty' and 'writable'. 
 *
 * nsIVariant users won't usually need to see this class.
 *
 * This class also has static helper methods that nsIVariant *implementors* can
 * use to help them do all the 'standard' nsIVariant data conversions.
 */

class nsVariant : public nsIWritableVariant
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIVARIANT
    NS_DECL_NSIWRITABLEVARIANT

    nsVariant();

    static nsresult Initialize(nsDiscriminatedUnion* data);
    static nsresult Cleanup(nsDiscriminatedUnion* data);

    static nsresult ConvertToInt8(const nsDiscriminatedUnion& data, PRUint8 *_retval);
    static nsresult ConvertToInt16(const nsDiscriminatedUnion& data, PRInt16 *_retval);
    static nsresult ConvertToInt32(const nsDiscriminatedUnion& data, PRInt32 *_retval);
    static nsresult ConvertToInt64(const nsDiscriminatedUnion& data, PRInt64 *_retval);
    static nsresult ConvertToUint8(const nsDiscriminatedUnion& data, PRUint8 *_retval);
    static nsresult ConvertToUint16(const nsDiscriminatedUnion& data, PRUint16 *_retval);
    static nsresult ConvertToUint32(const nsDiscriminatedUnion& data, PRUint32 *_retval);
    static nsresult ConvertToUint64(const nsDiscriminatedUnion& data, PRUint64 *_retval);
    static nsresult ConvertToFloat(const nsDiscriminatedUnion& data, float *_retval);
    static nsresult ConvertToDouble(const nsDiscriminatedUnion& data, double *_retval);
    static nsresult ConvertToBool(const nsDiscriminatedUnion& data, bool *_retval);
    static nsresult ConvertToChar(const nsDiscriminatedUnion& data, char *_retval NS_OUTPARAM);
    static nsresult ConvertToWChar(const nsDiscriminatedUnion& data, PRUnichar *_retval NS_OUTPARAM);
    static nsresult ConvertToID(const nsDiscriminatedUnion& data, nsID * _retval NS_OUTPARAM);
    static nsresult ConvertToAString(const nsDiscriminatedUnion& data, nsAString & _retval NS_OUTPARAM);
    static nsresult ConvertToAUTF8String(const nsDiscriminatedUnion& data, nsAUTF8String & _retval);
    static nsresult ConvertToACString(const nsDiscriminatedUnion& data, nsACString & _retval);
    static nsresult ConvertToString(const nsDiscriminatedUnion& data, char **_retval);
    static nsresult ConvertToWString(const nsDiscriminatedUnion& data, PRUnichar **_retval);
    static nsresult ConvertToISupports(const nsDiscriminatedUnion& data, nsISupports **_retval);
    static nsresult ConvertToInterface(const nsDiscriminatedUnion& data, nsIID * *iid NS_OUTPARAM, void * *iface NS_OUTPARAM);
    static nsresult ConvertToArray(const nsDiscriminatedUnion& data, PRUint16 *type NS_OUTPARAM, nsIID* iid NS_OUTPARAM, PRUint32 *count NS_OUTPARAM, void * *ptr);
    static nsresult ConvertToStringWithSize(const nsDiscriminatedUnion& data, PRUint32 *size NS_OUTPARAM, char **str);
    static nsresult ConvertToWStringWithSize(const nsDiscriminatedUnion& data, PRUint32 *size NS_OUTPARAM, PRUnichar **str);

    static nsresult SetFromVariant(nsDiscriminatedUnion* data, nsIVariant* aValue);

    static nsresult SetFromInt8(nsDiscriminatedUnion* data, PRUint8 aValue);
    static nsresult SetFromInt16(nsDiscriminatedUnion* data, PRInt16 aValue);
    static nsresult SetFromInt32(nsDiscriminatedUnion* data, PRInt32 aValue);
    static nsresult SetFromInt64(nsDiscriminatedUnion* data, PRInt64 aValue);
    static nsresult SetFromUint8(nsDiscriminatedUnion* data, PRUint8 aValue);
    static nsresult SetFromUint16(nsDiscriminatedUnion* data, PRUint16 aValue);
    static nsresult SetFromUint32(nsDiscriminatedUnion* data, PRUint32 aValue);
    static nsresult SetFromUint64(nsDiscriminatedUnion* data, PRUint64 aValue);
    static nsresult SetFromFloat(nsDiscriminatedUnion* data, float aValue);
    static nsresult SetFromDouble(nsDiscriminatedUnion* data, double aValue);
    static nsresult SetFromBool(nsDiscriminatedUnion* data, bool aValue);
    static nsresult SetFromChar(nsDiscriminatedUnion* data, char aValue);
    static nsresult SetFromWChar(nsDiscriminatedUnion* data, PRUnichar aValue);
    static nsresult SetFromID(nsDiscriminatedUnion* data, const nsID & aValue);
    static nsresult SetFromAString(nsDiscriminatedUnion* data, const nsAString & aValue);
    static nsresult SetFromAUTF8String(nsDiscriminatedUnion* data, const nsAUTF8String & aValue);
    static nsresult SetFromACString(nsDiscriminatedUnion* data, const nsACString & aValue);
    static nsresult SetFromString(nsDiscriminatedUnion* data, const char *aValue);
    static nsresult SetFromWString(nsDiscriminatedUnion* data, const PRUnichar *aValue);
    static nsresult SetFromISupports(nsDiscriminatedUnion* data, nsISupports *aValue);
    static nsresult SetFromInterface(nsDiscriminatedUnion* data, const nsIID& iid, nsISupports *aValue);
    static nsresult SetFromArray(nsDiscriminatedUnion* data, PRUint16 type, const nsIID* iid, PRUint32 count, void * aValue);
    static nsresult SetFromStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const char *aValue);
    static nsresult SetFromWStringWithSize(nsDiscriminatedUnion* data, PRUint32 size, const PRUnichar *aValue);

    static nsresult SetToVoid(nsDiscriminatedUnion* data);
    static nsresult SetToEmpty(nsDiscriminatedUnion* data);
    static nsresult SetToEmptyArray(nsDiscriminatedUnion* data);

    static void Traverse(const nsDiscriminatedUnion& data,
                         nsCycleCollectionTraversalCallback &cb);

private:
    ~nsVariant();

protected:
    nsDiscriminatedUnion mData;
    bool                 mWritable;
};

/**
 * Users of nsIVariant should be using the contractID and not this CID.
 *  - see NS_VARIANT_CONTRACTID in nsIVariant.idl.
 */

#define NS_VARIANT_CID \
{ /* 0D6EA1D0-879C-11d5-90EF-0010A4E73D9A */ \
    0xd6ea1d0,                               \
    0x879c,                                  \
    0x11d5,                                  \
    {0x90, 0xef, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a}}

#define NS_VARIANT_CLASSNAME "Variant"

#endif // nsVariant_h
