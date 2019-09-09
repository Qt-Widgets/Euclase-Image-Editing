#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QPoint>
#include <memory>
#include <functional>
#include <QMutex>

class Document {
public:
	enum class Type {
		Image,
		Block,
	};
	struct Header {
		unsigned int ref_ = 0;
		Type type_ = Type::Image;
		QPoint offset_;

		QPoint offset() const
		{
			return offset_;
		}
	};

	struct Image {
		Header header_;
		QImage image_;

		QPoint offset() const
		{
			return header_.offset();
		}

		void setOffset(QPoint const &pt)
		{
			header_.offset_ = pt;
		}

		void setOffset(int x, int y)
		{
			setOffset(QPoint(x, y));
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

		QPoint offset() const
		{
			return header_.offset();
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
//		Block *operator -> ()
//		{
//			return block();
//		}
//		Block const *operator -> () const
//		{
//			return block();
//		}
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
		static PanelPtr makeImage()
		{
			void *o = malloc(sizeof(Image));
			if (!o) throw std::bad_alloc();
			new(o) Image();
			PanelPtr p;
			p.assign(reinterpret_cast<Header *>(o));
			return p;
		}
		operator bool ()
		{
			return object_;
		}
	};

	class Layer {
	public:
		QPoint offset_;
		bool tile_mode_ = false;
		std::vector<PanelPtr> panels_;

		void clear(QMutex *sync)
		{
			if (sync) sync->lock();

			offset_ = QPoint();
			panels_.clear();

			if (sync) sync->unlock();
		}

		PanelPtr addImagePanel(int x = 0, int y = 0, int w = 64, int h = 64)
		{
			auto panel = PanelPtr::makeImage();
			panel->setOffset(x, y);
			if (w > 0 && h > 0) {
				panel->image_ = QImage(w, h, QImage::Format_RGBA8888);
				panel->image_.fill(Qt::transparent);
			}
			panels_.push_back(panel);
			std::sort(panels_.begin(), panels_.end(), [](PanelPtr const &l, PanelPtr const &r){
				auto COMP = [](PanelPtr const &l, PanelPtr const &r){
					if (l->offset().y() < r->offset().y()) return -1;
					if (l->offset().y() > r->offset().y()) return 1;
					if (l->offset().x() < r->offset().x()) return -1;
					if (l->offset().x() > r->offset().x()) return 1;
					return 0;
				};
				return COMP(l, r) < 0;
			});
			return panel;
		}

		Layer() = default;

		QPoint const &offset() const
		{
			return offset_;
		}

		void eachPanel(std::function<void(Image *)> const &fn)
		{
			for (PanelPtr &ptr : panels_) {
				fn(ptr.image());
			}
		}

		void setImage(QPoint const &offset, QImage const &image)
		{
			clear(nullptr);
			offset_ = offset;
			addImagePanel();
			panels_[0]->image_ = image;
		}

		QRect rect() const;
	};

	struct Private;
	Private *m;

	Document();
	~Document();

	int width() const;
	int height() const;
	QSize size() const;
	void setSize(QSize const &s);
	Layer *current_layer();
	Layer *selection_layer();
	Layer *current_layer() const;
	Layer *selection_layer() const;

	void paintToCurrentLayer(const Layer &source, const QColor &brush_color, QMutex *sync, bool *abort);

	QImage renderToLayer(QRect const &r, bool quickmask, QMutex *sync, bool *abort) const;
private:
	static void renderToEachPanels_(Image *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, bool *abort);
	static void renderToEachPanels(Image *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, QMutex *sync, bool *abort);
public:
	enum class SelectionOperation {
		SetSelection,
		AddSelection,
		SubSelection,
	};
	static void renderToSinglePanel(Image *target_panel, const QPoint &target_offset, const Image *input_panel, const QPoint &input_offset, const Layer *mask_layer, const QColor &brush_color, int opacity = 255, bool *abort = nullptr);
	static void renderToLayer(Layer *target_layer, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, QMutex *sync, bool *abort);
	void clearSelection(QMutex *sync);
	void addSelection(const Layer &source, QMutex *sync, bool *abort);
	void subSelection(const Layer &source, QMutex *sync, bool *abort);
	QImage renderSelection(const QRect &r, QMutex *sync, bool *abort) const;
	void changeSelection(SelectionOperation op, QRect const &rect, QMutex *sync);
	QImage crop(const QRect &r, QMutex *sync, bool *abort) const;
};

#endif // DOCUMENT_H
