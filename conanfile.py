from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import load, copy

class Generator(ConanFile):
  name = "Generator"
  version = "0.1"
  license = "MIT"
  author = "LectureCut"
  package_type = "application"
  settings = "os", "compiler", "build_type", "arch"

  requires = (
    "ffmpeg/5.1@#345de2be65b10978a2c94f5de14b0b56",
  )

  def generate(self):
    tc = CMakeToolchain(self)
    if self.settings.os == "Windows":
      tc.generator = "Visual Studio 17"
    tc.blocks["cppstd"].values = {"cppstd": "20"}
    tc.generate()

  def export_sources(self):
    copy(self, "*.txt", self.recipe_folder, self.export_sources_folder)
    copy(self, "src/*.cpp", self.recipe_folder, self.export_sources_folder)
    copy(self, "src/*.h", self.recipe_folder, self.export_sources_folder)

  def source(self):
    # Check that we can see that the CMakeLists.txt is inside the source folder
    load(self, "CMakeLists.txt")

  def configure(self):
    self.options["ffmpeg"].shared = False
    self.options["ffmpeg"].fPIC = True
    self.options["ffmpeg"].avdevice = False
    self.options["ffmpeg"].avcodec = True
    self.options["ffmpeg"].avformat = True
    self.options["ffmpeg"].swresample = True
    self.options["ffmpeg"].swscale = False
    self.options["ffmpeg"].postproc = False
    self.options["ffmpeg"].avfilter = False
    self.options["ffmpeg"].with_asm = False
    self.options["ffmpeg"].with_zlib = False
    self.options["ffmpeg"].with_bzip2 = False
    self.options["ffmpeg"].with_lzma = False
    self.options["ffmpeg"].with_libiconv = False
    self.options["ffmpeg"].with_freetype = False
    self.options["ffmpeg"].with_openjpeg = False
    self.options["ffmpeg"].with_openh264 = False
    self.options["ffmpeg"].with_opus = False
    self.options["ffmpeg"].with_vorbis = False
    self.options["ffmpeg"].with_zeromq = False
    self.options["ffmpeg"].with_sdl = False
    self.options["ffmpeg"].with_libx264 = False
    self.options["ffmpeg"].with_libx265 = False
    self.options["ffmpeg"].with_libvpx = False
    self.options["ffmpeg"].with_libmp3lame = False
    self.options["ffmpeg"].with_libfdk_aac = False
    self.options["ffmpeg"].with_libwebp = False
    self.options["ffmpeg"].with_ssl = False
    self.options["ffmpeg"].with_libalsa = False
    self.options["ffmpeg"].with_pulse = False
    self.options["ffmpeg"].with_vaapi = False
    self.options["ffmpeg"].with_vdpau = False
    self.options["ffmpeg"].with_xcb = False
    self.options["ffmpeg"].with_appkit = False
    self.options["ffmpeg"].with_avfoundation = False
    self.options["ffmpeg"].with_coreimage = False
    self.options["ffmpeg"].with_audiotoolbox = False
    self.options["ffmpeg"].with_videotoolbox = False
    self.options["ffmpeg"].with_programs = False
    

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()
