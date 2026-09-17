from PIL import Image
import os


root_texture_folder = 'Content/Textures/plant_tex'
bark_texture_folder = 'Content\Textures\plant_tex\colors\\bark'
leaf_texture_folder = 'Content\Textures\plant_tex\colors\leaves'
masks_folder = 'Content\Textures\plant_tex\masks'


def scale_down_x2_copy_x4(image : Image) -> Image:
    scaled_size = 128
    scaled_image = image.resize((scaled_size, scaled_size), Image.Dither.NONE)
    
    new_image = Image.new('RGBA', (512, 512))

    for x in range(0, 512, scaled_size):
        for y in range(0, 512, scaled_size):
            new_image.paste(scaled_image, (x, y))


    return new_image
    

def apply_transparency(branch_mask, leaf_mask, branch_color, leaf_color, output_path):
    # load textures and copy x4
    branch_texture = Image.open(branch_color).convert("RGBA")
    branch_texture = scale_down_x2_copy_x4(branch_texture)
    leaf_texture = Image.open(leaf_color).convert("RGBA")
    leaf_texture = scale_down_x2_copy_x4(leaf_texture)

    # load masks
    branch_alpha = Image.open(branch_mask).convert("L")  # Convert mask to grayscale
    leaf_alpha = Image.open(leaf_mask).convert("L")  # Convert mask to grayscale

    # apply mask to branch texture
    r, g, b, _ = branch_texture.split()
    branch_result = Image.merge("RGBA", (r, g, b, branch_alpha))

    # apply mask to leaf texture
    r, g, b, _ = leaf_texture.split()
    leaf_result = Image.merge("RGBA", (r, g, b, leaf_alpha))

    # combine
    combined_result = Image.alpha_composite(branch_result, leaf_result)
    combined_result.save(output_path)



def make_every_combination():

    # get all the masks
    masks = []
    for mask_folder in os.listdir(masks_folder):
        if os.path.isdir(f'{masks_folder}/{mask_folder}'):
            mask_files = os.listdir(f'{masks_folder}/{mask_folder}')
            if len(mask_files) == 2:
                branch_mask = mask_files[0]
                leaf_mask = mask_files[1]
                masks.append((branch_mask, leaf_mask, mask_folder))


    # get all the textures
    bark_textures = os.listdir(bark_texture_folder)
    leaf_textures = os.listdir(leaf_texture_folder)


    # make every combination
    for branch_mask, leaf_mask, mask_name in masks:
        for branch_color in bark_textures:
            for leaf_color in leaf_textures:
                br_name = branch_color.replace('.png', '')
                lf_name = leaf_color.replace('.png', '')
                output = f'{root_texture_folder}/{mask_name}__{br_name}_{lf_name}.png'
                apply_transparency(
                    f'{masks_folder}/{mask_name}/{branch_mask}',
                    f'{masks_folder}/{mask_name}/{leaf_mask}',
                    f'{bark_texture_folder}/{branch_color}',
                    f'{leaf_texture_folder}/{leaf_color}',
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
