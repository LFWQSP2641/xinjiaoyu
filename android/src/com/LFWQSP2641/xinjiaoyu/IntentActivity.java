package com.LFWQSP2641.xinjiaoyu;

import org.qtproject.qt.android.bindings.QtActivity;
import java.io.File;
import android.net.Uri;
import android.content.Intent;
import androidx.core.content.FileProvider;
import android.os.Build;

public class IntentActivity extends org.qtproject.qt.android.bindings.QtActivity
{
    public static void installApk(String filePath, QtActivity activity)
    {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        File apkFile = new File(filePath);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Uri contentUri = FileProvider.getUriForFile(activity, "com.LFWQSP2641.xinjiaoyu.permissiontest.fileprovider", apkFile);
            intent.setDataAndType(contentUri, "application/vnd.android.package-archive");
        }
        else
        {
            intent.setDataAndType(Uri.fromFile(apkFile), "application/vnd.android.package-archive");
        }
        activity.startActivity(intent);
    };
    public static void openUrl(String url, QtActivity activity)
    {
        Intent intent = new Intent();
        // 执行动作
        intent.setAction(Intent.ACTION_VIEW);
        // 执行的数据类型
        intent.setDataAndType(Uri.parse(url),"text/html");
        activity.startActivity(intent);
    };
};
