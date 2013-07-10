package com.openml.jni;

public class JEnv {
	/// setup the the jni environment
	public static native boolean setupJni(String apkDir, String params, Object assetManager);
	
	/// load the jni lib
	static{
		System.loadLibrary("env");
	}
}
