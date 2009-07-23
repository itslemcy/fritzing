import settings
import os 
from django.db import models
from django.contrib.auth.models import User
from django.db.models.signals import post_save
from django.utils.translation import ugettext_lazy as _

class Profile(models.Model):
    def project_images_path(self, filename):
        username = self.user.username
        path = "/".join(list(username[:3])) if len(username) >= 3 else ''
        return os.path.join(settings.USER_FILES_FOLDER, path, username, "profile", filename)
    
    user = models.ForeignKey(User, unique=True, verbose_name=_('user'))
    #name = models.CharField(_('name'), max_length=50, null=True, blank=True)
    about = models.TextField(_('about'), null=True, blank=True)
    location = models.CharField(_('location'), max_length=40, null=True, blank=True)
    website = models.URLField(_('website'), null=True, blank=True, verify_exists=False)
    image = models.ImageField(_('image'),upload_to=project_images_path,blank=True)
    
    def __unicode__(self):
        return self.user.username
    
    def get_absolute_url(self):
        return ('profile_detail', None, {'username': self.user.username})
    get_absolute_url = models.permalink(get_absolute_url)
    
    class Meta:
        verbose_name = _('profile')
        verbose_name_plural = _('profiles')

def create_profile(sender, instance=None, **kwargs):
    if instance is None:
        return
    profile, created = Profile.objects.get_or_create(user=instance)

post_save.connect(create_profile, sender=User)
