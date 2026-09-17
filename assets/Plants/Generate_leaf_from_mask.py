from PIL import Image
import os


texture_folder = 'assets/Plants/colors'
masks_folder = 'assets/Plants/masks'
output_folder = 'assets/Plants'


def scale_down_x2_copy_x4(image : Image) -> Image:
    scaled_size = 128
    scaled_image = image.resize((scaled_size, scaled_size), Image.Dither.NONE)
    
    new_image = Image.new('RGBA', (512, 512))

    for x in range(0, 512, scaled_size):
        for y in range(0, 512, scaled_size):
            new_image.paste(scaled_image, (x, y))


    return new_image
    

def apply_transparency(branch_mask, branch_color, output_path):
    # load textures and copy x4
    branch_texture = Image.open(branch_color).convert("RGBA")
    # branch_texture = scale_down_x2_copy_x4(branch_texture)

    # load masks
    branch_alpha = Image.open(branch_mask).convert("L")  # Convert mask to grayscale

    # apply mask to branch texture
    r, g, b, _ = branch_texture.split()
    branch_result = Image.merge("RGBA", (r, g, b, branch_alpha))

    # save
    branch_result.save(output_path)



def make_every_combination():

    # get all the masks
    masks = os.listdir(masks_folder)


    # get all the textures
    textures = os.listdir(texture_folder)

    # make every combination
    for mask in masks:
        for branch_color in textures:
            br_name = branch_color.replace('.png', '')
            mask_name = mask.replace('.png', '')
            output = f'{output_folder}/{mask_name}_{br_name}.png'
            apply_transparency(
                f'{masks_folder}/{mask}',
                f'{texture_folder}/{branch_color}',
                output
            )



    print("Done!")



make_every_combination()


# branch_mask = 'Content\Textures\plant_tex\masks\pine-bow-branch.png'
# leaf_mask = 'Content\Textures\plant_tex\masks\pine-bow-leaves.png'
# branch_color = 'Content\Textures\plant_tex\colors\\bark\\red_bark.png'
# leaf_color = 'Content\Textures\plant_tex\colors\leaves\mid_green_leaves.png'
# output = 'Content\Textures\plant_tex\pine_1.png'


# apply_transparency(
#     branch_mask,
#     leaf_mask,
#     branch_color,
#     leaf_color,
#     output
# )
