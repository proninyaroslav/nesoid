package com.androidemu.nes;

import static android.app.Activity.RESULT_OK;
import static com.androidemu.nes.EmulatorSettings.gameKeysName;
import static com.androidemu.nes.EmulatorSettings.gameKeysPref;
import static com.androidemu.nes.EmulatorSettings.gameKeysPref2;

import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.widget.Toast;

import com.androidemu.nes.wrapper.Wrapper;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;
import java.util.TreeMap;

public class EmulatorSettingsFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    private static final Uri ABOUT_URI = Uri.parse(
            "file:///android_asset/about.html");
    private static final String GITHUB_URI =
            "https://github.com/proninyaroslav/nesoid";
    private static final String GAME_GRIPPER_URI =
            "https://sites.google.com/site/gamegripper";

    private static final int REQUEST_LOAD_KEY_PROFILE = 1;
    private static final int REQUEST_SAVE_KEY_PROFILE = 2;
    private static final int REQUEST_GG_ROM = 100;
    private static final int REQUEST_FDS_ROM = 101;



    // FIXME
    private static ArrayList<String> getAllKeyPrefs() {
        ArrayList<String> result = new ArrayList<>();
        result.addAll(Arrays.asList(gameKeysPref));
        result.addAll(Arrays.asList(gameKeysPref2));
        return result;
    }

    private SharedPreferences settings;

    private Map<String, Integer> getKeyMappings() {
        TreeMap<String, Integer> mappings = new TreeMap<>();

        for (String key : getAllKeyPrefs()) {
            KeyPreference pref = (KeyPreference) findPreference(key);
            mappings.put(key, pref.getKeyValue());
        }
        return mappings;
    }

    private void setKeyMappings(Map<String, Integer> mappings) {
        SharedPreferences.Editor editor = settings.edit();

        for (String key : getAllKeyPrefs()) {
            Integer value = mappings.get(key);
            if (value != null) {
                KeyPreference pref = (KeyPreference) findPreference(key);
                pref.setKey(value);
                editor.putInt(key, value);
            }
        }
        editor.apply();
    }

    private void enableDisablePad2(String device) {
        findPreference("gamepad2").setEnabled("gamepad".equals(device));
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.preferences);

        settings = PreferenceManager.getDefaultSharedPreferences(getActivity());
        enableDisablePad2(settings.getString("secondController", "none"));

        // game genie rom
        String rom = settings.getString("gameGenieRom", null);
        if (rom != null)
            findPreference("gameGenieRom").setSummary(rom);

        // fds rom
        rom = settings.getString("fdsRom", null);
        if (rom != null)
            findPreference("fdsRom").setSummary(rom);

        final int[] defaultKeys = DefaultPreferences.getKeyMappings(getActivity());

        // gamepad 1
        PreferenceGroup group = (PreferenceGroup) findPreference("gamepad1");
        for (int i = 0; i < gameKeysPref.length; i++) {
            KeyPreference pref = new KeyPreference(getActivity());
            pref.setKey(gameKeysPref[i]);
            pref.setTitle(gameKeysName[i]);
            pref.setDefaultValue(defaultKeys[i]);
            group.addPreference(pref);
        }
        //  gamepad 2
        group = (PreferenceGroup) findPreference("gamepad2");
        for (int i = 0; i < gameKeysPref2.length; i++) {
            KeyPreference pref = new KeyPreference(getActivity());
            pref.setKey(gameKeysPref2[i]);
            pref.setTitle(gameKeysName[i]);
            group.addPreference(pref);
        }

        if (!Wrapper.supportsMultitouch(getActivity())) {
            findPreference("enableVKeypad").
                    setSummary(R.string.multitouch_unsupported);
        }

        findPreference("accurateRendering").setOnPreferenceChangeListener(this);
        findPreference("secondController").setOnPreferenceChangeListener(this);

        findPreference("about").setIntent(new Intent(
                getActivity(), HelpActivity.class).setData(ABOUT_URI));
        findPreference("github").setIntent(new Intent(
                Intent.ACTION_VIEW, Uri.parse(GITHUB_URI)));
        findPreference("searchRoms").setIntent(EmulatorSettings.getSearchROMIntent());

        findPreference("gameGripper").setIntent(new Intent(
                Intent.ACTION_VIEW, Uri.parse(GAME_GRIPPER_URI)));
    }

    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
            case REQUEST_GG_ROM:
                if (result == RESULT_OK) {
                    String rom = data.getData().getPath();
                    settings.edit().putString("gameGenieRom", rom).apply();
                    findPreference("gameGenieRom").setSummary(rom);
                }
                break;

            case REQUEST_FDS_ROM:
                if (result== RESULT_OK) {
                    String rom = data.getData().getPath();
                    settings.edit().putString("fdsRom", rom).apply();
                    findPreference("fdsRom").setSummary(rom);
                }
                break;

            case REQUEST_LOAD_KEY_PROFILE:
                if (result == RESULT_OK) {
                    setKeyMappings(KeyProfilesActivity.
                            loadProfile(getActivity(), data.getAction()));
                }
                break;

            case REQUEST_SAVE_KEY_PROFILE:
                if (result == RESULT_OK) {
                    KeyProfilesActivity.saveProfile(getActivity(), data.getAction(),
                            getKeyMappings());
                }
                break;
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
                                         Preference preference) {
        String key = preference.getKey();

        if ("gameGenieRom".equals(key)) {
            String path = settings.getString(key, null);
            Uri uri = null;
            if (path != null)
                uri = Uri.fromFile(new File(path));

            Intent intent = new Intent(getActivity(), FileChooser.class);
            intent.setData(uri);
            intent.putExtra(FileChooser.EXTRA_TITLE,
                    getResources().getString(R.string.game_genie_rom));
            intent.putExtra(FileChooser.EXTRA_FILTERS,
                    new String[] { ".nes", ".rom", ".bin" });

            startActivityForResult(intent, REQUEST_GG_ROM);
            return true;
        }
        if ("fdsRom".equals(key)) {
            String path = settings.getString(key, null);
            Uri uri = null;
            if (path != null)
                uri = Uri.fromFile(new File(path));

            Intent intent = new Intent(getActivity(), FileChooser.class);
            intent.setData(uri);
            intent.putExtra(FileChooser.EXTRA_TITLE,
                    getResources().getString(R.string.fds_rom));
            intent.putExtra(FileChooser.EXTRA_FILTERS,
                    new String[] { ".nes", ".rom", ".bin" });

            startActivityForResult(intent, REQUEST_FDS_ROM);
            return true;
        }

        if ("loadKeyProfile".equals(key)) {
            Intent intent = new Intent(getActivity(), KeyProfilesActivity.class);
            intent.putExtra(KeyProfilesActivity.EXTRA_TITLE,
                    getString(R.string.load_profile));
            startActivityForResult(intent, REQUEST_LOAD_KEY_PROFILE);
            return true;
        }
        if ("saveKeyProfile".equals(key)) {
            Intent intent = new Intent(getActivity(), KeyProfilesActivity.class);
            intent.putExtra(KeyProfilesActivity.EXTRA_TITLE,
                    getString(R.string.save_profile));
            startActivityForResult(intent, REQUEST_SAVE_KEY_PROFILE);
            return true;
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();

        if ("accurateRendering".equals(key)) {
            Toast.makeText(getActivity(), R.string.accurate_rendering_prompt,
                    Toast.LENGTH_SHORT).show();

        } else if ("secondController".equals(key))
            enableDisablePad2(newValue.toString());

        return true;
    }
}
