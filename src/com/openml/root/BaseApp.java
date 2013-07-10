package com.openml.root;

import android.app.Application;
import android.content.res.AssetManager;

import com.openml.jni.JEnv;

public class BaseApp extends Application {

	@Override
	public void onCreate() {
		super.onCreate();
		// init the global app
		BaseContext.gContext = this;
		// init the jni 
		AssetManager mgr = getAssets();
		JEnv.setupJni(getPackageResourcePath(), "", mgr);
    }
}
