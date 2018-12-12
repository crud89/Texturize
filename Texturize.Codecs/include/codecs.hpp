#pragma once

#include <texturize.hpp>
#include <analysis.hpp>
#include <filestorage.hpp>

#include <string>
#include <iostream>
#include <functional>
#include <typeindex>

namespace Texturize {
	using namespace Persistence;

	/// \defgroup codecs Persistence codecs
	/// Contains types that implement codecs for images and assets.
	/// @{
	
	/// \brief An interface that provides storing and restoring data from a file.
	class TEXTURIZE_API IFileStorage
	{
	public:
		/// \brief Writes data to a file, identified by a file name.
		/// \param name The full file name of the file to write the data to.
		/// \param _data The data to write to a file.
		virtual void write(const std::string &name, int _data) = 0;
		virtual void write(const std::string &name, float _data) = 0;
		virtual void write(const std::string &name, double _data) = 0;
		virtual void write(const std::string &name, const std::string &_data) = 0;
		virtual void write(const std::string &name, const cv::Mat &_data) = 0;
		virtual void write(const std::string &name, const cv::SparseMat &_data) = 0;
		virtual void write(const std::string &name, const std::vector<int> &_data) = 0;
		virtual void write(const std::string &name, const std::vector<float> &_data) = 0;
		virtual void write(const std::string &name, const std::vector<double> &_data) = 0;
		virtual void write(const std::string &name, const std::vector<std::string> &_data) = 0;
		virtual void write(const std::string &name, const std::vector<cv::KeyPoint> &_data) = 0;
		virtual void write(const std::string &name, const std::vector<cv::DMatch> &_data) = 0;
		virtual void write(const std::string &name, const cv::Range& _data) = 0;

		/// \brief Restored data from a file, identified by a file name.
		/// \param name The full file name of the file to read the data from.
		/// \param _data A reference of a buffer to restore the data to.
		virtual void read(const std::string &name, int &data) const = 0;
		virtual void read(const std::string &name, float &data) const = 0;
		virtual void read(const std::string &name, double &data) const = 0;
		virtual void read(const std::string &name, std::string &data) const = 0;
		virtual void read(const std::string &name, cv::Mat &data) const = 0;
		virtual void read(const std::string &name, cv::SparseMat &data) const = 0;
		virtual void read(const std::string &name, std::vector<int> &data) const = 0;
		virtual void read(const std::string &name, std::vector<float> &data) const = 0;
		virtual void read(const std::string &name, std::vector<double> &data) const = 0;
		virtual void read(const std::string &name, std::vector<std::string> &data) const = 0;
		virtual void read(const std::string &name, std::vector<cv::KeyPoint> &ks) const = 0;
		virtual void read(const std::string &name, std::vector<cv::DMatch> &dm) const = 0;
		virtual void read(const std::string &name, cv::Range& _data) const = 0;
	};

	/// \brief Creates a wrapper for a file storage implementation.
	/// \tparam TStorage The type of the storage to wrap.
	template <typename TStorage>
	class TEXTURIZE_API FileStorageWrapper :
		public IFileStorage
	{
	protected:
		TStorage _storage;

	public:
		FileStorageWrapper();
		FileStorageWrapper(const std::string &fileName, int flags, const std::string &encoding = std::string());
		~FileStorageWrapper();

	public:
		/// \copydoc Texturize::IFileStorage::write
		virtual void write(const std::string &name, int _data) override;
		virtual void write(const std::string &name, float _data) override;
		virtual void write(const std::string &name, double _data) override;
		virtual void write(const std::string &name, const std::string &_data) override;
		virtual void write(const std::string &name, const cv::Mat &_data) override;
		virtual void write(const std::string &name, const cv::SparseMat &_data) override;
		virtual void write(const std::string &name, const std::vector<int> &_data) override;
		virtual void write(const std::string &name, const std::vector<float> &_data) override;
		virtual void write(const std::string &name, const std::vector<double> &_data) override;
		virtual void write(const std::string &name, const std::vector<std::string> &_data) override;
		virtual void write(const std::string &name, const std::vector<cv::KeyPoint> &_data) override;
		virtual void write(const std::string &name, const std::vector<cv::DMatch> &_data) override;
		virtual void write(const std::string &name, const cv::Range& _data) override;

		/// \copydoc Texturize::IFileStorage::read
		virtual void read(const std::string &name, int &data) const override;
		virtual void read(const std::string &name, float &data) const override;
		virtual void read(const std::string &name, double &data) const override;
		virtual void read(const std::string &name, std::string &data) const override;
		virtual void read(const std::string &name, cv::Mat &data) const override;
		virtual void read(const std::string &name, cv::SparseMat &data) const override;
		virtual void read(const std::string &name, std::vector<int> &data) const override;
		virtual void read(const std::string &name, std::vector<float> &data) const override;
		virtual void read(const std::string &name, std::vector<double> &data) const override;
		virtual void read(const std::string &name, std::vector<std::string> &data) const override;
		virtual void read(const std::string &name, std::vector<cv::KeyPoint> &ks) const override;
		virtual void read(const std::string &name, std::vector<cv::DMatch> &dm) const override;
		virtual void read(const std::string &name, cv::Range& _data) const override;
	};

	/// \brief A file storage implementation, that uses OpenCV's file storage implementation to store files.
	///
	/// \see Texturize::IFileStorage
	class TEXTURIZE_API OpenCvFileStorage :
		public FileStorageWrapper<cv::FileStorage>
	{
	public:
		OpenCvFileStorage() : 
			FileStorageWrapper<cv::FileStorage>()
		{
		}

		OpenCvFileStorage(const std::string &fileName, int flags, const std::string &encoding = std::string()) :
			FileStorageWrapper<cv::FileStorage>(fileName, flags, encoding)
		{
		}
	};

	/// \brief A file storage implementation, that uses HDF5 to store files.
	///
	/// \see Texturize::IFileStorage
	class TEXTURIZE_API Hdf5FileStorage :
		public FileStorageWrapper<Texturize::Persistence::FileStorage>
	{
	public:
		Hdf5FileStorage() :
			FileStorageWrapper<Texturize::Persistence::FileStorage>()
		{
		}

		Hdf5FileStorage(const std::string &fileName, int flags, const std::string &encoding = std::string()) :
			FileStorageWrapper<Texturize::Persistence::FileStorage>(fileName, flags, encoding)
		{
		}
	};

	/// \brief The default storage factory that can be used to create a file storage.
	class TEXTURIZE_API StorageFactory
	{
	public:
		/// \brief The mode, the storage will use.
		enum StorageMode
		{
			FSM_READ = 0x01,
			FSM_WRITE = 0x02,
			FSM_UNDEFINED = INT_MAX
		};

		/// \brief The format of the storage.
		enum StorageFormat
		{
			// NOTE: Those are binary compatible to cv::FileStorage::Mode::FORMAT_*
			FSF_XML = 0x08,
			FSF_YAML = 0x10,
			FSF_JSON = 0x18,
			FSF_HDF5 = 0x20,
			FSF_UNDEFINED = INT_MAX
		};

	public:
		/// \brief Creates a file storage, based on a file name and format.
		virtual void createStorage(const std::string& fileName, IFileStorage** storage, StorageMode mode, StorageFormat format = StorageFormat::FSF_XML, const std::string& encoding = std::string()) const;
	};

	/// \brief An interface that is used to load and save images.
	class TEXTURIZE_API ISampleCodec
	{
	public:
		/// \brief Loads an image file.
		/// \param fileName The name of the image to load.
		/// \param sample The sample to restore the image to.
		virtual void load(const std::string& fileName, Sample& sample) const = 0;

		/// \brief Loads an image file.
		/// \param stream The stream to load the image from.
		/// \param sample The sample to restore the image to.
		virtual void load(std::istream& stream, Sample& sample) const = 0;

		/// \brief Saves an image to a file.
		/// \param fileName The name of the file, the image will be saved to.
		/// \param sample The sample that should be saved.
		/// \param depth The depth of the image file. Note that this is only supported for some formats.
		virtual void save(const std::string& fileName, const Sample& sample, const int depth = CV_8U) const = 0;

		/// \brief Saves an image to a file.
		/// \param stream The stream, the image should be written to.
		/// \param sample The sample that should be saved.
		/// \param depth The depth of the image file. Note that this is only supported for some formats.
		virtual void save(std::ostream& stream, const Sample& sample, const int depth = CV_8U) const = 0;
	};

	/// \brief Implements an image codec that uses OpenCV's `cv::imread` and `cv::imwrite` methods to read and write image files.
	class TEXTURIZE_API DefaultCodec :
		public ISampleCodec
	{
	public:
		virtual void load(const std::string& fileName, Sample& sample) const override;
		virtual void load(std::istream& stream, Sample& sample) const override;
		virtual void save(const std::string& fileName, const Sample& sample, const int depth = CV_8U) const override;
		virtual void save(std::ostream& stream, const Sample& sample, const int depth = CV_8U) const override;
	};

	/// \brief An object that allows to manage multiple codecs and dispatch persistence requests accordingly.
	class TEXTURIZE_API SamplePersistence
	{
	protected:
		typedef const ISampleCodec* const LPCSAMPLECODEC;

	protected:
		struct _caseInsensitiveStringCmp {
			bool operator() (const std::string& lhs, const std::string& rhs) const {
				return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
			}
		};

	protected:
		LPCSAMPLECODEC _defaultCodec = nullptr;
		std::map<std::string, LPCSAMPLECODEC, _caseInsensitiveStringCmp> _codecs;

	public:
		SamplePersistence() = default;
		SamplePersistence(LPCSAMPLECODEC defaultCodec);

	public:
		/// \brief Registers a new codec.
		/// \param extension The file extension to register the codec for.
		/// \param codec An instance of the codec interface.
		///
		/// \see ISampleCodec
		void registerCodec(const std::string& extension, LPCSAMPLECODEC codec);

		/// \brief Loads a sample from a file.
		/// \param fileName The file to load the sample from.
		/// \param sample A buffer to restore the sample to.
		void loadSample(const std::string& fileName, Sample& sample) const;

		/// \brief Loads a sample from a stream.
		/// \param stream The stream to load the sample from.
		/// \param extension The extension of a registered codec.
		/// \param sample A buffer to restore the sample to.
		void loadSample(std::istream& stream, const std::string& extension, Sample& sample) const;

		/// \brief Saves a sample to a file.
		/// \param fileName The name of the file to save the sample to.
		/// \param sample The sample to save.
		/// \param depth The depth of the image file.
		void saveSample(const std::string& fileName, const Sample& sample, const int depth = CV_8U) const;

		/// \brief Writes a sample to a stream.
		/// \param stream The stream to write the sample to.
		/// \param sample The sample to save.
		/// \param depth The depth of the written output channels.
		void saveSample(std::ostream& stream, const std::string& extension, const Sample& sample, const int depth = CV_8U) const;
	};

	/// \copydoc Texturize::SamplePersistence
	template <typename TDefaultCodec = DefaultCodec>
	class TEXTURIZE_API SamplePersistence_ :
		public SamplePersistence
	{
		static_assert(std::is_base_of<ISampleCodec, TDefaultCodec>::value, "The default codec type must implement the 'ISampleCodec' class.");

	public:
		SamplePersistence_();
		virtual ~SamplePersistence_();
	};

	typedef TEXTURIZE_API SamplePersistence_<> DefaultPersistence;

	/// \brief An object that is used to store/restore assets to/from an file storage.
	/// \tparam TAsset The type of the asset, the object will store or restore.
	template <typename TAsset>
	class TEXTURIZE_API AssetPersistence
	{
	public:
		/// \brief Writes an asset to a file storage.
		/// \param storage The storage to write the asset to.
		/// \param asset The asset to write to the storage.
		virtual void store(IFileStorage* storage, const TAsset* asset) const = 0;

		/// \brief Restores an asset from a file storage.
		/// \param storage The storage to restore the asset from.
		/// \param asset The asset to restore from the storage.
		virtual void restore(const IFileStorage* storage, TAsset** asset) const = 0;
	};

	template <typename TAsset>
	class TEXTURIZE_API FunctionalAssetPersistence :
		public AssetPersistence<TAsset>
	{
	public:
		typedef std::function<void(IFileStorage*, const TAsset*)> ASSET_WRITER;
		typedef std::function<void(const IFileStorage*, TAsset**)> ASSET_READER;

	private:
		ASSET_WRITER _writerFunc;
		ASSET_READER _readerFunc;

	public:
		FunctionalAssetPersistence(ASSET_WRITER writer, ASSET_READER reader);

	public:
		virtual void store(IFileStorage* storage, const TAsset* asset) const override;
		virtual void restore(const IFileStorage* storage, TAsset** asset) const override;
	};

	/// \brief An object that allows to store `AppearanceSpace` instances inside an asset.
	class TEXTURIZE_API AppearanceSpaceAsset :
		public AssetPersistence<AppearanceSpace>
	{
	public:
		virtual void store(IFileStorage* storage, const AppearanceSpace* asset) const override;
		virtual void restore(const IFileStorage* storage, AppearanceSpace** asset) const override;

		void write(const std::string& fileName, const AppearanceSpace* descriptor, const StorageFactory& storages = StorageFactory()) const;
		void read(const std::string& fileName, AppearanceSpace** descriptor, const StorageFactory& storages = StorageFactory()) const;
	};

	/// @}
}

#include "persistence.hpp"

// TODO: Example for implementing custom codecs
// TODO: Example for using persistence codecs