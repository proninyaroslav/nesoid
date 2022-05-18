package com.androidemu.nes;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceActivity;

import com.androidemu.Emulator;

public class EmulatorSettings extends PreferenceActivity {

	private static final String SEARCH_ROM_URI =
			"http://romfind.com/roms/nes/a.html";

	public static final int[] gameKeys = {
			Emulator.GAMEPAD_UP,
			Emulator.GAMEPAD_DOWN,
			Emulator.GAMEPAD_LEFT,
			Emulator.GAMEPAD_RIGHT,
			Emulator.GAMEPAD_UP_LEFT,
			Emulator.GAMEPAD_UP_RIGHT,
			Emulator.GAMEPAD_DOWN_LEFT,
			Emulator.GAMEPAD_DOWN_RIGHT,
			Emulator.GAMEPAD_SELECT,
			Emulator.GAMEPAD_START,
			Emulator.GAMEPAD_A,
			Emulator.GAMEPAD_B,
			Emulator.GAMEPAD_A_TURBO,
			Emulator.GAMEPAD_B_TURBO,
			Emulator.GAMEPAD_AB,
	};

	public static final String[] gameKeysPref = {
			"gamepad_up",
			"gamepad_down",
			"gamepad_left",
			"gamepad_right",
			"gamepad_up_left",
			"gamepad_up_right",
			"gamepad_down_left",
			"gamepad_down_right",
			"gamepad_select",
			"gamepad_start",
			"gamepad_A",
			"gamepad_B",
			"gamepad_A_turbo",
			"gamepad_B_turbo",
			"gamepad_AB",
	};

	public static final String[] gameKeysPref2 = {
			"gamepad2_up",
			"gamepad2_down",
			"gamepad2_left",
			"gamepad2_right",
			"gamepad2_up_left",
			"gamepad2_up_right",
			"gamepad2_down_left",
			"gamepad2_down_right",
			"gamepad2_select",
			"gamepad2_start",
			"gamepad2_A",
			"gamepad2_B",
			"gamepad2_A_turbo",
			"gamepad2_B_turbo",
			"gamepad2_AB",
	};

	public static final int[] gameKeysName = {
			R.string.gamepad_up,
			R.string.gamepad_down,
			R.string.gamepad_left,
			R.string.gamepad_right,
			R.string.gamepad_up_left,
			R.string.gamepad_up_right,
			R.string.gamepad_down_left,
			R.string.gamepad_down_right,
			R.string.gamepad_select,
			R.string.gamepad_start,
			R.string.gamepad_A,
			R.string.gamepad_B,
			R.string.gamepad_A_turbo,
			R.string.gamepad_B_turbo,
			R.string.gamepad_AB,
	};

	static {
		final int n = gameKeys.length;
		//noinspection ConstantConditions
		if (gameKeysPref.length != n ||
				gameKeysPref2.length != n ||
				gameKeysName.length != n)
			throw new AssertionError("Key configurations are not consistent");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setTitle(R.string.settings);
		getFragmentManager()
				.beginTransaction()
				.replace(android.R.id.content, new EmulatorSettingsFragment())
				.commit();
	}

	public static Intent getSearchROMIntent() {
		return new Intent(Intent.ACTION_VIEW, Uri.parse(SEARCH_ROM_URI)).
				setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	}
}
