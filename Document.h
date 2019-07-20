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
		std::shared_ptr<Panel> panel;

		Layer()
		{
			panel = std::make_shared<Panel>();
		}

		int width() const
		{
			return panel ? panel->image_.width() : 0;
		}

		int height() const
		{
			return panel ? panel->image_.height() : 0;
		}

		void eachPanel(std::function<void(Panel *)> fn)
		{
			fn(panel.get());
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
			return panel->image_;
		}
		QPoint &offset()
		{
			return panel->offset_;
		}

		QImage const &image() const
		{
			return panel->image_;
		}
		QPoint const &offset() const
		{
			return panel->offset_;
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
	static QImage renderLayer(const QRect &r, Layer const &current_layer, const QImage &selection_layer);
	static void blend_(const Layer &input_layer, const QColor &brush_color, Layer::Panel *target_panel, Layer *mask_layer);
	static void renderMask(QImage *dstimg, const QRect &r, const QImage &selimg);
public:
	static void blend(const Layer &input_layer, const QColor &brush_color, Layer *target_layer, Layer *mask_layer);
};

#endif // DOCUMENT_H
