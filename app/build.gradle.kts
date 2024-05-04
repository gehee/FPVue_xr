plugins {
    alias(libs.plugins.androidApplication)
}

android {
    namespace = "com.geehe.fpvue_xr"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.geehe.fpvue_xr"
        minSdk = 28
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"


        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.add("-DANDROID_PLATFORM=24") //24
                arguments.add("-DANDROID_STL=c++_shared")
                cppFlags.add("-std=c++20")
            }
        }
        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    externalNativeBuild {
        cmake {
            version = "3.22.1"
            path("src/main/cpp/CMakeLists.txt")
        }
    }
}

dependencies {
    implementation(fileTree("libs").include("*.jar"))
    implementation(libs.appcompat)
}