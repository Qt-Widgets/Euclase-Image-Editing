#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QPoint>
#include <memory>
#include <functional>

class Document {
public:
	class Layer {
	public:
		struct Panel {
			QPoint offset_;
			QImage image_;

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
			addPanel();
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

		bool isRGBA8888() const
		{
			return image().format() == QImage::Format_RGBA8888;
		}

		bool isGrayscale8() const
		{
			return image().format() == QImage::Format_Grayscale8;
		}

		QImage &image()
		{
			return panels[0]->image_;
		}
		QPoint &offset()
		{
			return panels[0]->offset_;
		}

		QImage const &image() const
		{
			return panels[0]->image_;
		}
		QPoint const &offset() const
		{
			return panels[0]->offset_;
		}
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

	void paint(const Layer &sel, const QColor &brush_color);

	QImage render(QRect const &r) const;
private:
	static void renderSelection(QImage *dstimg, const QRect &r, const QImage &selimg);
	static QImage renderLayer(const QRect &r, Layer const &current_layer, const QImage &selection);
	static void blend_(const Layer::Panel *input_panel, const QColor &brush_color, Layer::Panel *target_panel, Layer *mask_layer);
	static void blend_(const Layer &input_layer, const QColor &brush_color, Layer::Panel *target_panel, Layer *mask_layer);
	static void renderMask(QImage *dstimg, const QRect &r, const QImage &selimg);
public:
	static void blend(const Layer &input_layer, const QColor &brush_color, Layer *target_layer, Layer *mask_layer);
};

#endif // DOCUMENT_H
