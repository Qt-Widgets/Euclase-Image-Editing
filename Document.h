#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QPoint>
#include <memory>
#include <functional>
#include <QMutex>

class Document {
public:
	class Layer {
	public:
		enum class Type {
			Image,
			Block,
		};
		struct Header {
			unsigned int ref_ = 0;
			Type type_ = Type::Image;
		};

		struct Image {
			Header header_;
			QPoint offset_;
			QImage image_;

			QPoint offset() const
			{
				return offset_;
			}

			int width() const
			{
				return image_.width();
			}

			int height() const
			{
				return image_.height();
			}

			bool isRGBA8888() const
			{
				return image_.format() == QImage::Format_RGBA8888;
			}

			bool isGrayscale8() const
			{
				return image_.format() == QImage::Format_Grayscale8;
			}
		};
		struct Block {
			Header header_;
			int x = 0;
			int y = 0;

			QPoint offset() const
			{
				return QPoint(x * 64, y * 64);
			}
		};

		class PanelPtr {
		private:
			Header *object_ = nullptr;
		public:
			PanelPtr()
			{
			}
			PanelPtr(PanelPtr const &r)
			{
				assign(r.object_);
			}
			void operator = (PanelPtr const &r)
			{
				assign(r.object_);
			}
			~PanelPtr()
			{
				reset();
			}
			void reset()
			{
				assign(nullptr);
			}
			void assign(Header *p)
			{
				if (p == object_) {
					return;
				}
				if (p) {
					p->ref_++;
				}
				if (object_) {
					if (object_->ref_ > 1) {
						object_->ref_--;
					} else {
						switch (object_->type_) {
						case Type::Image:
							reinterpret_cast<Image *>(object_)->~Image();
							break;
						case Type::Block:
							reinterpret_cast<Block *>(object_)->~Block();
							break;
						}
						free(reinterpret_cast<void *>(object_));
					}
				}
				object_ = p;
			}

			Image *image()
			{
				if (object_ && reinterpret_cast<Header *>(object_)->type_ == Type::Image) {
					return reinterpret_cast<Image *>(object_);
				}
				return nullptr;
			}
			Image const *image() const
			{
				if (object_ && reinterpret_cast<Header *>(object_)->type_ == Type::Image) {
					return reinterpret_cast<Image *>(object_);
				}
				return nullptr;
			}
			Image *operator -> ()
			{
				return image();
			}
			Image const *operator -> () const
			{
				return image();
			}
			operator Image *()
			{
				return image();
			}
			operator const Image *() const
			{
				return image();
			}

			Block *block()
			{
				if (object_ && reinterpret_cast<Header *>(object_)->type_ == Type::Block) {
					return reinterpret_cast<Block *>(object_);
				}
				return nullptr;
			}
			Block const *block() const
			{
				if (object_ && reinterpret_cast<Header *>(object_)->type_ == Type::Block) {
					return reinterpret_cast<Block *>(object_);
				}
				return nullptr;
			}
//			Block *operator -> ()
//			{
//				return block();
//			}
//			Block const *operator -> () const
//			{
//				return block();
//			}
			operator Block *()
			{
				return block();
			}
			operator const Block *() const
			{
				return block();
			}

			bool isImage() const
			{
				return image();
			}
			bool isBlock() const
			{
				return block();
			}
			static PanelPtr makePanel()
			{
				void *o = malloc(sizeof(Image));
				if (!o) throw std::bad_alloc();
				new(o) Image();
				PanelPtr p;
				p.assign(reinterpret_cast<Header *>(o));
				return p;
			}
		};

		std::vector<PanelPtr> panels;

		int width_ = 0;
		int height_ = 0;
		QPoint offset_;

		void clear(QMutex *sync)
		{
			if (sync) sync->lock();

			width_ = height_ = 0;
			offset_ = QPoint();
			panels.clear();

			if (sync) sync->unlock();
		}

		PanelPtr addPanel()
		{
			auto panel = PanelPtr::makePanel();
			panels.push_back(panel);
			return panel;
		}

		Layer(int w = 0, int h = 0)
			: width_(w)
			, height_(h)
		{
		}

		void create(int w, int h)
		{
			width_ = w;
			height_ = h;
			panels.clear();
			for (int y = 0; y < h; y += 64) {
				for (int x = 0; x < w; x += 64) {
					auto panel = addPanel();
					panel->image_ = QImage(64, 64, QImage::Format_RGBA8888);
					panel->image_.fill(Qt::transparent);
					panel->offset_ = QPoint(x, y);
				}
			}
		}

		QPoint const &offset() const
		{
			return offset_;
		}

		int width() const
		{
			return width_;
		}

		int height() const
		{
			return height_;
		}

		QSize size() const
		{
			return QSize(width(), height());
		}

		void eachPanel(std::function<void(Image *)> fn)
		{
			for (PanelPtr &ptr : panels) {
				fn(ptr.image());
			}
		}

		void setImage(QPoint const &offset, QImage const &image)
		{
			clear(nullptr);
			offset_ = offset;
			addPanel();
			panels[0]->image_ = image;
		}

		QRect rect() const;
	};

	struct Private;
	Private *m;

	Document();
	~Document();

	int width() const;
	int height() const;
	Layer *current_layer();
	Layer *selection_layer();
	Layer *current_layer() const;
	Layer *selection_layer() const;

	void paintToCurrentLayer(const Layer &source, const QColor &brush_color, QMutex *sync, bool *abort);

	QImage renderToLayer(QRect const &r, bool quickmask, QMutex *sync, bool *abort) const;
private:
	static void renderToEachPanels_(Layer::Image *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, bool *abort);
	static void renderToEachPanels(Layer::Image *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, QMutex *sync, bool *abort);
	static void renderToSinglePanel(Layer::Image *target_panel, const QPoint &target_offset, const Layer::Image *input_panel, const QPoint &input_offset, const Layer *mask_layer, const QColor &brush_color, int opacity = 255, bool *abort = nullptr);
public:
	enum class SelectionOperation {
		SetSelection,
		AddSelection,
		SubSelection,
	};
	static void renderToLayer(Layer *target_layer, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, QMutex *sync, bool *abort);
	void clearSelection(QMutex *sync);
	void addSelection(const Layer &source, QMutex *sync, bool *abort);
	void subSelection(const Layer &source, QMutex *sync, bool *abort);
	QImage renderSelection(const QRect &r, QMutex *sync, bool *abort) const;
	void changeSelection(SelectionOperation op, QRect const &rect, QMutex *sync);
};

#endif // DOCUMENT_H
