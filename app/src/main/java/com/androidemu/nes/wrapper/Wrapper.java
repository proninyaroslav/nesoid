package com.androidemu.nes.wrapper;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.pm.PackageManager;
import android.view.MotionEvent;

public class Wrapper {

	public static boolean isBluetoothPresent() {
		return (BluetoothAdapter.getDefaultAdapter() != null);
	}

	public static boolean isBluetoothEnabled() {
		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		return (adapter != null && adapter.isEnabled());
	}

	public static boolean isBluetoothDiscoverable() {
		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
		if (adapter == null)
			return false;
		return (adapter.getScanMode() == BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE);
	}

	public static boolean supportsMultitouch(Context context) {
		return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH);
	}

	public static int MotionEvent_getPointerCount(MotionEvent event) {
		return event.getPointerCount();
	}

	public static int MotionEvent_getPointerId(MotionEvent event, int pointerIndex) {
		return event.getPointerId(pointerIndex);
	}

	public static int MotionEvent_findPointerIndex(MotionEvent event, int pointerId) {
		return event.findPointerIndex(pointerId);
	}

	public static float MotionEvent_getX(MotionEvent event, int pointerIndex) {
		return event.getX(pointerIndex);
	}

	public static float MotionEvent_getY(MotionEvent event, int pointerIndex) {
		return event.getY(pointerIndex);
	}

	public static float MotionEvent_getSize(MotionEvent event, int pointerIndex) {
		return event.getSize(pointerIndex);
	}
}
