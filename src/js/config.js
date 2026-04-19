module.exports = [
  {
    type: 'heading',
    defaultValue: 'Settings'
  },
  {
    type: 'select',
    messageKey: 'LANGUAGE',
    label: 'Language',
    defaultValue: 'en',
    options: [
      { label: 'English', value: 'en' },
      { label: 'Deutsch', value: 'de' },
      { label: 'Français', value: 'fr' },
      { label: 'Italiano', value: 'it' }
    ]
  },
  {
    type: 'heading',
    defaultValue: 'Favorite Stations'
  },
  {
    type: 'text',
    id: 'station-picker',
    defaultValue: ''
  },
  {
    type: 'input',
    messageKey: 'FAVORITES',
    label: 'Favorites',
    defaultValue: '[]'
  },
  {
    type: 'toggle',
    messageKey: 'DISRUPTIONS',
    label: 'Show disruption notices',
    defaultValue: true
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
