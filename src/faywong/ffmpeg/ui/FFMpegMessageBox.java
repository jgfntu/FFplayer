package faywong.ffmpeg.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;

public class FFMpegMessageBox {
	private static AlertDialog dialog;
	
	public static void show(Context context, String title, String msg) {
		dialog = new AlertDialog.Builder(context)  
        .setMessage(msg)  
        .setTitle(title)  
        .setCancelable(true)  
        .setNeutralButton(android.R.string.ok,  
           new DialogInterface.OnClickListener() {  
           public void onClick(DialogInterface dialog, int whichButton){}  
           })  
        .show();
	}
	
	public static void show(Context context, Exception ex) {
		show(context, "Error", ex.getMessage());
	}
	
	public static void reCycle() {
		if (null != dialog) {
			dialog.dismiss();
		}
	}

}
