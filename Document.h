#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QPoint>
#include <memory>
#include <functional>
#include <QMutex>

class Synchronize {
public:
	QMutex mutex;
};

class Document {
public:
	class Layer {
	public:
		struct Panel {
			QPoint offset_;
			QImage image_;

			QPoint offset() const
			{
				return offset_;
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
		using PanelPtr = std::shared_ptr<Panel>;
		std::vector<PanelPtr> panels;

		int width_ = 0;
		int height_ = 0;
		QPoint offset_;

		void clear(Synchronize *sync)
		{
			if (sync) sync->mutex.lock();

			width_ = height_ = 0;
			offset_ = QPoint();
			panels.clear();

			if (sync) sync->mutex.unlock();
		}

		PanelPtr addPanel()
		{
			auto panel = std::make_shared<Panel>();
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

		void eachPanel(std::function<void(Panel *)> fn)
		{
			for (PanelPtr &ptr : panels) {
				fn(ptr.get());
			}
		}

		void setImage(QPoint const &offset, QImage const &image)
		{
			clear(nullptr);
			addPanel();
			offset_ = offset;
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

	void paintToCurrentLayer(const Layer &source, const QColor &brush_color, Synchronize *sync, bool *abort);

	QImage renderToLayer(QRect const &r, bool quickmask, Synchronize *sync, bool *abort) const;
private:
	static void renderToEachPanels_(Layer::Panel *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, bool *abort);
	static void renderToEachPanels(Layer::Panel *target_panel, const QPoint &target_offset, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, int opacity, Synchronize *sync, bool *abort);
	static void renderToSinglePanel(Layer::Panel *target_panel, const QPoint &target_offset, const Layer::Panel *input_panel, const QPoint &input_offset, const Layer *mask_layer, const QColor &brush_color, int opacity = 255, bool *abort = nullptr);
public:
	enum class SelectionOperation {
		SetSelection,
		AddSelection,
		SubSelection,
	};
	static void renderToLayer(Layer *target_layer, const Layer &input_layer, Layer *mask_layer, const QColor &brush_color, Synchronize *sync, bool *abort);
	void clearSelection(Synchronize *sync);
	void addSelection(const Layer &source, Synchronize *sync, bool *abort);
	void subSelection(const Layer &source, Synchronize *sync, bool *abort);
	QImage renderSelection(const QRect &r, Synchronize *sync, bool *abort) const;
	void changeSelection(SelectionOperation op, QRect const &rect, Synchronize *sync);
};

#endif // DOCUMENT_H
