#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QPoint>

class Document {
public:
	class Layer {
	public:
		QPoint offset;
		QImage image;

		bool isRGBA8888() const
		{
			return image.format() == QImage::Format_RGBA8888;
		}

		bool isGrayscale8() const
		{
			return image.format() == QImage::Format_Grayscale8;
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
	static void blend(const Layer &input_layer, const QColor &brush_color, Layer *target_layer, Layer *mask_layer);
	static void renderMask(QImage *dstimg, const QRect &r, const QImage &selimg);
};

#endif // DOCUMENT_H
