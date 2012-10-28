package me.icechip.android.lib.mediaplayer;

import java.io.File;
import java.util.ArrayList;

import android.content.Context;
import android.content.res.Resources;
import android.view.View;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

/**
 * 
 * @author chips
 * @todo Always display Up button?
 * @todo Favorites?
 */
public class FileBrowser extends LinearLayout {
	private DirectoryView mDirectoryView;
	private LinearLayout mHeaderPanel;
	private HorizontalScrollView mHeaderScrollView;
	private Button mParentButton;
	private Button mRootButton;
	private OnClickListener mParentPathOnClickListener = new OnClickListener() {
        public void onClick(View v) {
        	File path = FileBrowser.this.mDirectoryView.getPath();
        	if(path != null) {
        		FileBrowser.this.mDirectoryView.setPath(path.getParentFile());
        	}
        }
    };
	private OnClickListener mButtonTagPathOnClickListener = new OnClickListener() {
        public void onClick(View v) {
        	File path = (File)v.getTag();
        	if(path != null) {
        		FileBrowser.this.mDirectoryView.setPath(path);
        	}
        }
    };
    private int mButtonTextLimit = 8;
	private boolean mInit = false;

	public FileBrowser(Context context) {
		super(context);
		init();
	}
	
	public File getPath() {
		return mDirectoryView.getPath();
	}
	
	public void setPath(File path) {
		mDirectoryView.setPath(path);
	}
	
	public boolean trySetPath(String path) {
		File file = new File(path);
		if(file.exists()) {
			setPath(file);
			return true;
		}
		return false;
	}
	
	protected void init() {
		final Context ctx = getContext();
		final Resources res = getResources();
        mHeaderPanel = new LinearLayout(ctx);
        mHeaderPanel.setOrientation(HORIZONTAL);
        mHeaderScrollView = new HorizontalScrollView(ctx);
        mHeaderScrollView.addView(mHeaderPanel, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
        
        mParentButton = new Button(ctx);
        // FIXME mParentButton.setText(res.getString(R.string.parent_dir));
        mParentButton.setText("Up");
        mParentButton.setOnClickListener(mParentPathOnClickListener);

        mRootButton = new Button(ctx);
        // FIXME mRootButton.setText(res.getString(R.string.root_dir));
        mRootButton.setText("Root");
        mRootButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
            	FileBrowser.this.mDirectoryView.setPath(null);
            }
        });
        
		mDirectoryView = new DirectoryView(ctx) {
			protected void onPathChanged() {
				super.onPathChanged();
				if(FileBrowser.this.mInit) {
					FileBrowser.this.updateHeader();
				}
			}
			protected void onOpenFile(File file) {
				FileBrowser.this.onOpenFile(file);
			}
		};

		setOrientation(VERTICAL);
        addView(mHeaderScrollView, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
        addView(mDirectoryView, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT, 1));
        
        mInit = true;
        updateHeader();
	}

	protected void onOpenFile(File file) {
		
	}
	
	// TODO: We should keep subdirs when navigating to parent folder (like nautilus & most other fmgrs do)
	// TODO: Also hide middle paths (pagination like)
	protected void updateHeader() {
		Context ctx = getContext();
		ArrayList<View> views = new ArrayList<View>();
		
		mHeaderPanel.removeAllViews();
        mHeaderPanel.addView(mParentButton);
        mHeaderPanel.addView(mRootButton);
        
		for(File path = mDirectoryView.getPath(); path != null; path = path.getParentFile()) {
			String name = path.getName();
			if(name.length() > 0) {
				if(name.length() > mButtonTextLimit) {
					name = name.substring(0, mButtonTextLimit - 3) + "...";
				}
				Button btn = new Button(ctx);
				btn.setText(name);
				btn.setOnClickListener(mButtonTagPathOnClickListener);
				btn.setTag(path);
				views.add(0, btn);
			}
		}
		
		for(View view : views) {
			mHeaderPanel.addView(view);
		}
	}

}
