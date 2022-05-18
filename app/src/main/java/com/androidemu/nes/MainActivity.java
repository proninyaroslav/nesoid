package com.androidemu.nes;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import java.io.File;

public class MainActivity extends FileChooser {

	private static final Uri HELP_URI = Uri.parse("file:///android_asset/faq.html");

	private static final int DIALOG_SHORTCUT = 1;

	private static Intent emulatorIntent;
	private SharedPreferences settings;
	private boolean creatingShortcut;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		settings = getSharedPreferences("MainActivity", MODE_PRIVATE);

		super.onCreate(savedInstanceState);
		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		setTitle(R.string.title_select_rom);

		creatingShortcut = getIntent().getAction().equals(
				Intent.ACTION_CREATE_SHORTCUT);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);

		if (!creatingShortcut)
			getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int itemId = item.getItemId();
		if (itemId == R.id.menu_search_roms) {
			startActivity(EmulatorSettings.getSearchROMIntent());
			return true;
		} else if (itemId == R.id.menu_help) {
			startActivity(new Intent(this, HelpActivity.class).
					setData(HELP_URI));
			return true;
		} else if (itemId == R.id.menu_settings) {
			startActivity(new Intent(this, EmulatorSettings.class));
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_SHORTCUT) {
			return createShortcutNameDialog();
		}
		return super.onCreateDialog(id);
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		if (id == DIALOG_SHORTCUT) {
			String name = emulatorIntent.getData().getPath();
			name = new File(name).getName();
			int dot = name.lastIndexOf('.');
			if (dot > 0)
				name = name.substring(0, dot);

			EditText v = ((EditText) dialog.findViewById(R.id.name));
			v.setText(name);
			v.selectAll();
		}
	}

	@Override
	protected String getInitialPath() {
		return settings.getString("lastGame", null);
	}

	@Override
	protected String[] getFileFilter() {
		return getResources().getStringArray(R.array.file_chooser_filters);
	}

	@Override
	protected void onFileSelected(Uri uri) {
		// remember the last file
		settings.edit().putString("lastGame", uri.getPath()).apply();

		Intent intent = new Intent(Intent.ACTION_VIEW, uri, this, EmulatorActivity.class);
		if (!creatingShortcut)
			startActivity(intent);
		else {
			emulatorIntent = intent;
			showDialog(DIALOG_SHORTCUT);
		}
	}

	private Dialog createShortcutNameDialog() {
		DialogInterface.OnClickListener l =
                (dialog, which) -> {
                    final Dialog d = (Dialog) dialog;
                    String name = ((EditText) d.findViewById(R.id.name)).getText().toString();

                    Intent intent = new Intent();
                    intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, emulatorIntent);
                    intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, name);
                    Parcelable icon = Intent.ShortcutIconResource.fromContext(MainActivity.this, R.mipmap.ic_launcher);
                    intent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, icon);

                    setResult(RESULT_OK, intent);
                    finish();
                };

		return new AlertDialog.Builder(this).
				setTitle(R.string.shortcut_name).
				setView(View.inflate(this, R.layout.shortcut, null)).
				setPositiveButton(android.R.string.ok, l).
				setNegativeButton(android.R.string.cancel, null).
				create();
	}
}
