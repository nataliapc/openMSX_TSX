#ifndef IMGUI_MARKDOWN_HH
#define IMGUI_MARKDOWN_HH

#include <imgui_md.h>
#include <string_view>

namespace openmsx {

class ImGuiLayer;

class ImGuiMarkdown : public imgui_md
{
public:
	ImGuiMarkdown(ImGuiLayer& layer_)
		: layer(layer_) {}

	void print(std::string_view);

	ImFont* get_font() const override;
	void open_url() const override;
	//bool get_image(image_info& nfo) const override;
	//void html_div(const std::string& divClass, bool e) override;

private:
	ImGuiLayer& layer;
};

}

#endif
