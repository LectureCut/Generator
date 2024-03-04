<div align="center" style="display: grid; grid-template-columns: 1fr 100px 1fr; gap: 10px">
  <span></span>
  <img src=".github/static/Logo.svg" width=100/>
</div>

---
<div align="center">
  <h3>If you like this project, please consider giving it a star ⭐️!</h3>
</div>

## 📝 Description

The LectureCut Generator is a module of the LectureCut project. It is responsible for analyzing the lecture video to find parts where no one is speaking. From these parts, a cut list is generated, which can be used by the render module to cut the lecture video into its final form.

## 🏗 Structure

Internally the Generator uses [`avcodec`](https://ffmpeg.org/libavcodec.html) and [`avformat`](https://ffmpeg.org/libavformat.html) from the [FFmpeg](https://ffmpeg.org/) project to extract the audio from the video and [`libfvad`](https://github.com/dpirch/libfvad) to detect speech in the audio. The Generator is written in [C++](https://isocpp.org/) to allow fast and efficient processing of videos.

<!-- license -->
## 📜 License

The LectureCut Generator is licensed under the [MIT License](LICENSE).