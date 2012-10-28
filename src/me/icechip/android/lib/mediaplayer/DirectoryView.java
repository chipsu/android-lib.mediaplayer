package me.icechip.android.lib.mediaplayer;

import java.io.File;
import java.io.FileFilter;
import java.util.Arrays;
import java.util.Comparator;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class DirectoryView extends ListView {
	private File mPath;
	private FileFilter mFileFilter = new DefaultFileFilter();
	private boolean mDirectoriesFirst = true;
	private boolean mShowHiddenFiles = false;

	public DirectoryView(Context context) {
		super(context);
		this.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
				File file = (File)arg1.getTag();
				if(file != null) {
					if(file.isDirectory()) {
						DirectoryView.this.setPath(file);
					} else {
						DirectoryView.this.onOpenFile(file);
					}
				}		
				
			}
		});
		onPathChanged();
	}

	public void setPath(File path) {
		if(mPath != path) {
			mPath = path;
			onPathChanged();
		}
	}
	
	public File getPath() {
		return mPath;
	}
	
	public void setFileFilter(FileFilter fileFilter) {
		if(mFileFilter != fileFilter) {
			mFileFilter = fileFilter;
			updateFileList();
		}
	}
	
	public FileFilter getFileFilter() {
		return mFileFilter;
	}
	
	public void updateFileList() {
		File[] files = null;
		if(mPath != null) {
			files = mPath.listFiles(mFileFilter);
		} else {
			files = java.io.File.listRoots();
			if(files.length == 1) {
				files = files[0].listFiles(mFileFilter);
			}
		}
		if(files != null) {
			sortFiles(files);
			ListAdapter adapter = new FileListAdapter(getContext(), files);
			setAdapter(adapter);
		} else {
			setAdapter(null);
		}
	}
	
	class DefaultFileFilter implements FileFilter {
		@Override
		public boolean accept(File pathname) {
			if(!mShowHiddenFiles) {
				return !pathname.isHidden();
			}
			return false;
		}
	}
	
	class FileListAdapter extends BaseAdapter {
		private Context mContext;
		private File[] mFiles;
		
		public FileListAdapter(Context context, File[] files) {
			mContext = context;
			mFiles = files;
		}

		@Override
		public int getCount() {
			return mFiles.length;
		}

		@Override
		public Object getItem(int position) {
			return mFiles[position];
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			File file = mFiles[position];
			String name = file.getName();
			TextView view = new TextView(mContext);
			//view.setText("@" + position + ": " + file.getName() + " > " + file.getAbsolutePath() + " (" + file.isDirectory() + ")");
			if(file.isDirectory()) {
				name = "[DIR] " + name; // FIXME
			}
			view.setText(name);
			view.setPadding(10, 10, 10, 10);
			view.setTag(file);
			return view;
		}

        @Override
        public boolean areAllItemsEnabled() {
            return true;
        }

        @Override
        public boolean isEnabled(int position) {
            return true;
        }

        @Override
        public int getItemViewType(int position) {
            // Don't let ListView try to reuse the views.
            return AdapterView.ITEM_VIEW_TYPE_IGNORE;
        }
	}
	
	class FileComparator implements Comparator<File> {
		private boolean mDirectoriesFirst = true;
		public FileComparator() {}
		public FileComparator(boolean directoriesFirst) {
			mDirectoriesFirst = directoriesFirst;
		}
        public int compare(File a, File b) {
            if(mDirectoriesFirst && a.isDirectory() != b.isDirectory()) {
            	return a.isDirectory() ? -1 : 1;
            }
            return a.getName().compareTo(b.getName());
        }
	}
	
	protected void sortFiles(File[] files) {
		Arrays.sort(files, new FileComparator(mDirectoriesFirst));
	}
	
	protected void onPathChanged() {
		updateFileList();
	}
	
	protected void onOpenFile(File file) {
		
	}
}
