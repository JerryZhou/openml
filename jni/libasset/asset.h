#include <jni.h>
#include <android/asset_manager_jni.h>

#ifndef __ASSET_H_
#define __ASSET_H_

/*********************************************************/

/**
 * entry of asset, 0 is invalid
 * */
typedef int AssetEntry;

/**
 * */
struct AssetDescriptor{
	int fd;
	off_t start;
	off_t length;
};

/**
 *
 * */
enum AssetOption{
	ASSET_GET_LENGTH, // int
	ASSET_SEEK_POS, // int[2]
	ASSET_CLOSE, // null
	ASSET_GET_ANDROID_DESCRIPTOR, // get the descriptor
};

/**
 * accroding to : SEEK_SET, SEEK_CUR, SEEK_END
 * */
enum AssetSet{
	ASSERT_SET,
	ASSERT_CUR,
	ASSERT_END,
};

/**
 * startup asset package, utf-8 encode
 * */
int asset_startup(const char* packagename, AAssetManager* assetMgr = NULL);

/**
 * kind of asset io module : read
 * If the file is successfully opened,
 *  the function returns a entry to a FILE object that
 *  can be used to identify the stream on future operations.
 *  Otherwise, a null entry(0) is returned.
 * */
AssetEntry asset_open(const char* face);

/**
 * if the asset is exit in package
 * */
bool asset_isExist(const char* face);

/**
 * @param return read lenth, when reach the end of file return -1
 * The total amount of bytes read if successful
 * */
int asset_read(AssetEntry entry, void* buffer, int len);
int asset_read_zip(AssetEntry entry, void* buffer, int len);

/**
 * seek to pos,
 * If successful, the function returns zero.
 * Otherwise, it returns non-zero value.
 * */
int asset_seek(AssetEntry entry, int pos, int set);

/**
 * close the asset entry
 * If the stream is successfully closed, a zero value is returned.
 *  On failure, EOF is returned.
 * */
int asset_close(AssetEntry entry);

/**
 * do option about the entry
 * */
int asset_option(AssetEntry entry, int option, void * value, void* key = NULL);

/**
 * android asset manager
 * */
AAssetManager* aasset_getAssetManager();

/**
 * android asset open
 * */
AAsset * android_asset_open(const char* file, int mode = AASSET_MODE_UNKNOWN);

/**
 * android asset read
 * */
int asset_read(AAsset* asset, void* buffer, int bufferSize);

/**
 * android asset length
 * */
int asset_length(AAsset* asset);

/**
 * android asset seek
 * */
int asset_seek(AAsset* asset, int pos, int set);

/**
 * android asset close
 * */
void asset_close(AAsset* asset);

/**
 * */
#define safe_close_asset(entry) if(entry != 0) { asset_close(entry); entry = 0; }

#endif
