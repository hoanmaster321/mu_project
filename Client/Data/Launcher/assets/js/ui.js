$(function(){

			var b_controls = true;
			var b_auto = true;
			var img_length = $('.visual_slider li').length;
			if(img_length == 1)	
			{
				b_controls = false;
				b_auto = false;
			}
			slider_main_roll = $('.visual_slider').bxSlider({mode: 'fade',auto:b_auto,pause:4000,speed:1000,useCSS: false,pager:false,controls:b_controls});
			$('.bx-pager').bind({click:function(){slider_main_roll.stopAuto();slider_main_roll.startAuto();}});
			
		
	});
$(function () {
	var placeholder_input = $(".input_div_wrp input:first-child, .input_div_wrp textarea:first-child");
	placeholder_input.bind({
		focusin : function(){
			var _this = $(this);
			//_this.addClass("Input_has_focus");
			_this.parent().find(".input_label").hide();
			//alert("포커스인")
		},
		focusout: function(){
			var _this = $(this);
			//_this.removeClass("Input_has_focus");
			if (!_this.val()) {
				_this.parent().find(".input_label").show();
			}
			//alert("포커스아웃")
		}
	}).not("[value='']").each(function(idx) {
		var _this = $(this);
		//_this.addClass("Input_has_focus");
		_this.parent().find(".input_label").hide();
		//alert("낫")
	});
});