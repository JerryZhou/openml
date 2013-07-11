#include "asset.h"
#include <zip.h>
#include <zipint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstddef>
#include "../include/log.h"
#include "../include/mutex.h"

// config information
#define MAX_ASSET_ENTRY 20
#define ENTRY_RESERVED 1 // must greater than 1
#define MAX_FACE_LEN 256
#define MAX_PKG_PATH_LEN 1024

// environment
char packageName[MAX_PKG_PATH_LEN] = {0};
AAssetManager* assetManager;
zip   *pkg_zip;
ThreadMutex assetMutex;

/**
 * only support to open 20 entry the same time
 * */
struct AssetEntryInternal{
	FILE* file;
	zip_file *zipFile;
	int offset;
	int length;
	int cur;
	char face[MAX_FACE_LEN];
}assetsEntrys[MAX_ASSET_ENTRY+ENTRY_RESERVED];

/**
 * not thread safe
 * */
static int get_valid_entry(){
	AutoLock autoLock(assetMutex);

	for(int i=ENTRY_RESERVED; i<__countof(assetsEntrys); ++i){
		if(assetsEntrys[i].file == NULL){
			return i;
		}
	}
	return 0;
}

/**
 * get entry of asset
 * */
static AssetEntryInternal& dataEntry(int idx){
	IF_DO(idx > __countof(assetsEntrys) || idx < ENTRY_RESERVED, idx = 0);
	return assetsEntrys[idx];
}

/**
 * is valid entry
 * */
static bool isEntryValid(int idx ){
	IF_DO(idx > __countof(assetsEntrys), return false);
	IF_DO(idx < ENTRY_RESERVED, return false);
	return true;
}

/**
 * */
int asset_startup(const char* buffer, AAssetManager* assetMgr){
	AutoLock autoLock(assetMutex);

	int error;
    pkg_zip = zip_open( buffer, 0, &error );
    strcpy( packageName, buffer );
    if( pkg_zip == NULL ){
       JB_LOGE("Failed to open apk: %i", error );
       return -1;
    }
    JB_LOGI("asset successful load: %s", buffer);
    memset(assetsEntrys, 0, sizeof(assetsEntrys));
    JB_LOGI("accept the assetManager");
    assetManager = assetMgr;
    return 0;
}

/**
 * kind of asset io module : read
 * */
AssetEntry asset_open(const char* face){
	J_ASSERT(strlen(face) < MAX_FACE_LEN);
	int idx = get_valid_entry();
	IF_DO(idx==0, return idx );

	AutoLock autoLock(assetMutex);
	AssetEntryInternal& entry = dataEntry(idx);

	zip_file *zfile = zip_fopen( pkg_zip, face, 0 );
	uint32_t offset = 0;
	uint32_t length = 0;

	if( zfile != NULL ) {
		offset = zfile->fpos;
		length = zfile->bytes_left;

		entry.zipFile = zfile;
		zfile = NULL;

		entry.length = length;
		entry.offset = offset;
		strcpy(entry.face, face);
	} else {
		return 0; // should known , 0 is invalid entry
	}

	FILE *fp = NULL;
	fp = fopen( packageName, "rb" );
	fseek( fp, offset, SEEK_SET );
	entry.file = fp;

	return idx;
}

/**
 * if the asset is exit in package
 * */
bool asset_isExist(const char* face){
	IF_DO(pkg_zip==NULL, return false);
	int result = zip_name_locate( pkg_zip, face, 0);
	return result != -1;
}

/**
 * @param return read lenth
 * */
int asset_read(AssetEntry asset, void* buffer, int len){
	IF_DO(!isEntryValid(asset), return 0);

	AutoLock autoLock(assetMutex);
	AssetEntryInternal& entry = dataEntry(asset);
	IF_DO(entry.file == NULL, return 0);
	IF_DO(entry.cur >= entry.length, return -1);

	len = clamp(len, 0, entry.length - entry.cur);
	return (int)fread(buffer, 1, len, entry.file);
}

/**
 * @param return read lenth
 * */
int asset_read_zip(AssetEntry asset, void* buffer, int len){
	IF_DO(!isEntryValid(asset), return 0);

	AutoLock autoLock(assetMutex);
	AssetEntryInternal& entry = dataEntry(asset);
	IF_DO(entry.file == NULL, return 0);

	return (int)zip_fread(entry.zipFile, buffer, len);
}


/**
 * seek to pos
 * */
int asset_seek(AssetEntry asset, int pos, int set){
	IF_DO(!isEntryValid(asset), return -1);

	AutoLock autoLock(assetMutex);
	AssetEntryInternal& entry = dataEntry(asset);
	IF_DO(entry.file == NULL, return -1);

	if(set == ASSERT_SET){// Beginning of file
		pos = clamp(pos, 0, entry.length);
		entry.cur = pos;
	}else if(set == ASSERT_CUR){// Current position of the file pointer
		pos = clamp(pos, -entry.cur, entry.length - entry.cur);
		entry.cur = entry.cur + pos;
	}else if(set == ASSERT_END){
		pos = clamp(pos, -entry.length, 0);
		entry.cur = entry.length + pos;
	}

	//calcute the new pos relative to beiginning of file
	pos = (entry.offset + entry.cur);
	return fseek( entry.file, pos, SEEK_SET );
}

/**
 * close the asset entry
 * */
int asset_close(AssetEntry asset){
	IF_DO(!isEntryValid(asset), return EOF);

	AutoLock autoLock(assetMutex);
	AssetEntryInternal& entry = dataEntry(asset);
	IF_DO(entry.file == NULL, return EOF);

	zip_fclose(entry.zipFile);
	FILE* close_file = entry.file;
	entry.file = NULL;
	return fclose(close_file);
}

/**
 * do option about the entry
 * */
int asset_option(AssetEntry asset, int option, void * value,  void* key){
	AutoLock autoLock(assetMutex);

	if(option == ASSET_GET_LENGTH){
		IF_DO(!isEntryValid(asset), return -1);
		AssetEntryInternal& entry = dataEntry(asset);
		IF_DO(entry.file == NULL, return -1);

		return entry.length;
	}else if(option == ASSET_SEEK_POS){
		int *vs = (int*)(value);
		return asset_seek(asset, vs[0], vs[1]);
	}else if(option == ASSET_CLOSE){
		return asset_close(asset);
	}else if(option == ASSET_GET_ANDROID_DESCRIPTOR){
		IF_DO(key == NULL, return -1);
		IF_DO(assetManager == NULL, return -1);

		const char* path = (const char*)(key);
		const char* face = strstr(path, "assets/") == path ? path + strlen("assets/") : path;
		AssetDescriptor* descriptor = (AssetDescriptor*)(value);
		AAsset* lAsset = AAssetManager_open(assetManager, face , AASSET_MODE_UNKNOWN);
        if (lAsset != NULL) {
        	descriptor->fd = AAsset_openFileDescriptor(
                lAsset, &descriptor->start, &descriptor->length);
            AAsset_close(lAsset);
            return 0;
        }
	}

	return -1;
}

/**
 * android asset manager
 * */
AAssetManager* aasset_getAssetManager(){
	return assetManager;
}

/**
 * android asset open
 * */
AAsset * android_asset_open(const char* file, int mode){
	const char* face = strstr(file, "assets/") == file ? file + strlen("assets/") : file;
	return AAssetManager_open(assetManager, face, mode);
}

/**
 * android asset read
 * */
int asset_read(AAsset* asset, void* buffer, int bufferSize){
	return AAsset_read(asset, buffer, bufferSize);
}

/**
 * android asset length
 * */
int asset_length(AAsset* asset){
	return AAsset_getLength(asset);
}

/**
 * android asset seek
 * */
int asset_seek(AAsset* asset, int pos, int set){
	return AAsset_seek(asset, pos, set);
}

/**
 * android asset close
 * */
void asset_close(AAsset* asset){
	AAsset_close(asset);
}
