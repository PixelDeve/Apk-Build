package com.pixelcraft.go;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.android.gms.auth.api.signin.*;
import com.google.android.gms.common.api.ApiException;
import com.google.firebase.auth.*;
import com.google.android.gms.tasks.Task;

/**
 * MainActivity — hosts the SurfaceView and Google Sign-In.
 * The C++ game loop runs on a dedicated GL thread.
 */
public class MainActivity extends Activity implements SurfaceHolder.Callback {

    // ── Native methods ────────────────────────────────────────────
    public native void nativeInit(Surface surface, int w, int h);
    public native void nativeResize(int w, int h);
    public native void nativeFrame();
    public native void nativeDestroy();

    // ── JNI callback from C++ auth result ────────────────────────
    public native void onAuthResult(String uid, String email, String name,
                                     String photo, String token,
                                     boolean ok, String err);

    // ── Google Sign-In ────────────────────────────────────────────
    private static final int RC_SIGN_IN = 9001;
    private GoogleSignInClient mGoogleSignInClient;
    private FirebaseAuth       mFirebaseAuth;

    // ── GL thread ─────────────────────────────────────────────────
    private GLThread   glThread;
    private SurfaceView surfaceView;
    private boolean    surfaceReady = false;

    static { System.loadLibrary("voxelpocket"); }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Fullscreen immersive
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );

        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);

        // Firebase Auth
        mFirebaseAuth = FirebaseAuth.getInstance();

        // Google Sign-In config
        GoogleSignInOptions gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
            .requestIdToken(getString(R.string.default_web_client_id))
            .requestEmail()
            .build();
        mGoogleSignInClient = GoogleSignIn.getClient(this, gso);

        // Check if already signed in
        FirebaseUser user = mFirebaseAuth.getCurrentUser();
        if (user != null) {
            user.getIdToken(true).addOnSuccessListener(res -> {
                notifyAuthSuccess(user, res.getToken());
            });
        }
    }

    // ── Called from C++ via GoogleAuth.cpp ───────────────────────
    public void startGoogleSignIn() {
        Intent signInIntent = mGoogleSignInClient.getSignInIntent();
        startActivityForResult(signInIntent, RC_SIGN_IN);
    }

    public void signOut() {
        mFirebaseAuth.signOut();
        mGoogleSignInClient.signOut();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == RC_SIGN_IN) {
            Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
            try {
                GoogleSignInAccount account = task.getResult(ApiException.class);
                firebaseAuthWithGoogle(account.getIdToken());
            } catch (ApiException e) {
                // Call C++ with failure
                onAuthResult("","","","","", false, e.getMessage());
            }
        }
    }

    private void firebaseAuthWithGoogle(String idToken) {
        AuthCredential cred = GoogleAuthProvider.getCredential(idToken, null);
        mFirebaseAuth.signInWithCredential(cred)
            .addOnCompleteListener(this, task -> {
                if (task.isSuccessful()) {
                    FirebaseUser user = mFirebaseAuth.getCurrentUser();
                    user.getIdToken(true).addOnSuccessListener(res -> {
                        notifyAuthSuccess(user, res.getToken());
                    });
                } else {
                    onAuthResult("","","","","", false,
                        task.getException() != null ? task.getException().getMessage() : "Auth failed");
                }
            });
    }

    private void notifyAuthSuccess(FirebaseUser user, String token) {
        onAuthResult(
            user.getUid(),
            user.getEmail() != null ? user.getEmail() : "",
            user.getDisplayName() != null ? user.getDisplayName() : "",
            user.getPhotoUrl() != null ? user.getPhotoUrl().toString() : "",
            token != null ? token : "",
            true, ""
        );
    }

    // ── SurfaceHolder callbacks ───────────────────────────────────
    @Override
    public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        if (glThread == null) {
            glThread = new GLThread(holder.getSurface(), w, h);
            glThread.start();
        } else {
            nativeResize(w, h);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (glThread != null) {
            glThread.requestStop();
            glThread = null;
        }
    }

    @Override
    protected void onResume() { super.onResume(); if (glThread != null) glThread.resume(); }

    @Override
    protected void onPause() { super.onPause(); if (glThread != null) glThread.pause(); }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (glThread != null) { glThread.requestStop(); glThread = null; }
    }

    // ── GL thread ─────────────────────────────────────────────────
    private class GLThread extends Thread {
        private final Surface surface;
        private final int w, h;
        private volatile boolean running  = true;
        private volatile boolean paused   = false;

        GLThread(Surface s, int w, int h) { this.surface=s; this.w=w; this.h=h; }

        @Override
        public void run() {
            nativeInit(surface, w, h);
            while (running) {
                if (!paused) nativeFrame();
                try { Thread.sleep(1); } catch (InterruptedException ignored){}
            }
            nativeDestroy();
        }

        void requestStop() { running = false; }
        void pause()  { paused = true;  }
        void resume() { paused = false; }
    }
}
